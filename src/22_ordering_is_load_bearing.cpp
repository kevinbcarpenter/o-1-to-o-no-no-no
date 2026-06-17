// Round 2: Pitfall 0 - "unordered" means unordered - Acquirer Edition
// Distilled from a real auth service: a per-request map is iterated IN ORDER
// to build a delimited wire message, which is then hashed for integrity. The
// receiver re-derives the hash from a canonical (sorted) field order.
//
// Swap that std::map for an unordered_map and the field order changes, the
// bytes change, the hash changes -> the receiver rejects the message. A single
// fixed-payload unit test will NOT catch it.
//
// Patterns drawn from a real payment-auth service; code here is distilled.

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

constexpr char FIELD_SEP = '\x1C';   // like the service's MML field separator

// Stable, platform-independent integrity hash (FNV-1a 64) so the demo is
// reproducible anywhere. Stands in for the service's SHA1 over the payload.
std::uint64_t fnv1a(const std::string& s) {
    std::uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    return h;
}

// Build the wire message by iterating whatever container is passed in.
template <typename Map>
std::string build_message(const Map& fields) {
    std::string msg;
    for (const auto& [tag, value] : fields) {
        msg += tag;
        msg += value;
        msg += FIELD_SEP;
    }
    return msg;
}

std::string visible(const std::string& s) {
    std::string out;
    for (char c : s) out += (c == FIELD_SEP) ? '|' : c;
    return out;
}

int main() {
    std::cout << "=== Pitfall 0: it's in the name (unordered == unordered) ===\n\n";

    // The same logical request, as (tag, value) pairs.
    const std::vector<std::pair<std::string, std::string>> request = {
        {"AUTH_GUID", "A1B2"}, {"MERCH_NBR", "300078"}, {"TRAN_TYPE", "SALE"},
        {"AMOUNT", "5000"},    {"CARD_BIN", "411111"},  {"STAN", "000123"},
        {"CURRENCY", "USD"},   {"TERM_ID", "TERM07"},   {"BATCH_ID", "42"},
        {"TRACE", "ZZ9"},
    };

    std::map<std::string, std::string> ordered(request.begin(), request.end());
    std::unordered_map<std::string, std::string> unordered(request.begin(), request.end());

    // The RECEIVER's contract: hash is computed over canonical (sorted) order.
    const std::uint64_t canonical_hash = fnv1a(build_message(ordered));

    auto ordered_msg   = build_message(ordered);
    auto unordered_msg = build_message(unordered);
    auto ordered_hash   = fnv1a(ordered_msg);
    auto unordered_hash = fnv1a(unordered_msg);

    std::cout << "std::map field order:\n  " << visible(ordered_msg) << "\n";
    std::cout << "  hash = " << ordered_hash
              << (ordered_hash == canonical_hash ? "  ACCEPTED\n\n" : "  REJECTED\n\n");

    std::cout << "std::unordered_map field order:\n  " << visible(unordered_msg) << "\n";
    std::cout << "  hash = " << unordered_hash
              << (unordered_hash == canonical_hash ? "  ACCEPTED\n\n" : "  REJECTED  <-- wire protocol broken!\n\n");

    std::cout << "Same data, same code path, one container swap. The map's iteration\n";
    std::cout << "order WAS the wire format. unordered_map scrambled it and broke the hash.\n";
    std::cout << "No bad hash, no collision, no rehash. A pure correctness pitfall.\n";
    return 0;
}
