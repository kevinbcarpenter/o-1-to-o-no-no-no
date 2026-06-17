// Section 5: Reference & Iterator Stability - Acquirer Edition
// The node-based tax you pay for std::unordered_map buys you one thing:
// pointers and references survive a rehash. Iterators do NOT.
// Flat / open-addressing maps give up BOTH. That's the speed trade-off.
//
// Patterns drawn from a real payment-auth service; code here is distilled.

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

struct AuthRecord {
    std::string auth_code;
    uint32_t amount_cents;
    std::string status;
};

int main() {
    std::cout << "=== Reference & Iterator Stability Across Containers ===\n\n";

    // ---------------------------------------------------------------
    // 1. std::unordered_map: references STABLE, iterators NOT
    // ---------------------------------------------------------------
    std::cout << "--- std::unordered_map: the node-based guarantee ---\n\n";
    {
        std::unordered_map<std::string, AuthRecord> auth_cache;
        auth_cache.reserve(4);

        auth_cache["TXN_0001"] = {"AUTH_A1B2", 5000, "APPROVED"};
        auth_cache["TXN_0002"] = {"AUTH_C3D4", 12599, "APPROVED"};

        // Reversal handler caches a pointer AND an iterator
        AuthRecord* ptr = &auth_cache["TXN_0001"];
        auto iter = auth_cache.find("TXN_0001");

        std::cout << "Before rehash:\n";
        std::cout << "  ptr  -> " << ptr << "  auth_code: " << ptr->auth_code << "\n";
        std::cout << "  iter -> " << &(iter->second) << "  auth_code: " << iter->second.auth_code << "\n";
        std::cout << "  bucket_count: " << auth_cache.bucket_count() << "\n\n";

        // Flood new auths to force a rehash
        std::cout << "Inserting 18 more auths to trigger rehash...\n";
        for (int i = 3; i <= 20; ++i) {
            auth_cache["TXN_" + std::to_string(i)] =
                {"AUTH_" + std::to_string(i), uint32_t(i * 1000), "APPROVED"};
        }
        std::cout << "  bucket_count: " << auth_cache.bucket_count() << "\n\n";

        std::cout << "After rehash:\n";
        std::cout << "  ptr  -> " << ptr << "  auth_code: " << ptr->auth_code
                  << "  ** STILL VALID, same address! **\n";
        // iter is now INVALIDATED. Dereferencing it is undefined behavior.
        // We verify the pointer independently:
        std::cout << "  &map[key] = " << &auth_cache["TXN_0001"]
                  << "  (matches ptr: "
                  << (ptr == &auth_cache["TXN_0001"] ? "YES" : "NO") << ")\n\n";

        std::cout << "  Pointer/reference: STABLE  (the standard requires this)\n";
        std::cout << "  Iterator:          INVALIDATED by rehash (UB to dereference)\n\n";
        std::cout << "  This is WHY unordered_map uses node-based allocation.\n";
        std::cout << "  Each element lives in its own heap node; a rehash rewires\n";
        std::cout << "  bucket pointers but never moves the nodes themselves.\n\n";
    }

    // ---------------------------------------------------------------
    // 2. std::map: EVERYTHING stable (references, pointers, iterators)
    // ---------------------------------------------------------------
    std::cout << "--- std::map: full stability ---\n\n";
    {
        std::map<std::string, AuthRecord> auth_cache;

        auth_cache["TXN_0001"] = {"AUTH_A1B2", 5000, "APPROVED"};
        auth_cache["TXN_0002"] = {"AUTH_C3D4", 12599, "APPROVED"};

        AuthRecord* ptr = &auth_cache["TXN_0001"];
        auto iter = auth_cache.find("TXN_0001");

        for (int i = 3; i <= 20; ++i) {
            auth_cache["TXN_" + std::to_string(i)] =
                {"AUTH_" + std::to_string(i), uint32_t(i * 1000), "APPROVED"};
        }

        std::cout << "  ptr  -> " << ptr->auth_code << "\n";
        std::cout << "  iter -> " << iter->second.auth_code << "\n";
        std::cout << "  Both valid: pointers, references, AND iterators survive insert.\n\n";
    }

    // ---------------------------------------------------------------
    // 3. Flat / open-addressing maps: NOTHING stable
    // ---------------------------------------------------------------
    std::cout << "--- Flat / open-addressing maps: the speed trade-off ---\n\n";
    {
        // Simulate with a sorted vector (same layout as flat_map / flat hash maps)
        std::vector<std::pair<std::string, AuthRecord>> flat;
        flat.push_back({"TXN_0001", {"AUTH_A1B2", 5000, "APPROVED"}});
        flat.push_back({"TXN_0002", {"AUTH_C3D4", 12599, "APPROVED"}});
        auto by_key = [](const auto& a, const auto& b) { return a.first < b.first; };
        std::sort(flat.begin(), flat.end(), by_key);

        AuthRecord* ptr = &flat[0].second;
        std::cout << "  ptr before growth -> " << ptr << "\n";

        for (int i = 3; i <= 20; ++i) {
            flat.push_back({"TXN_" + std::to_string(i),
                {"AUTH_" + std::to_string(i), uint32_t(i * 1000), "APPROVED"}});
        }
        std::sort(flat.begin(), flat.end(), by_key);

        std::cout << "  ptr after growth  -> " << ptr << "  ** DANGLING **\n";
        std::cout << "  &flat[0].second   -> " << &flat[0].second << "  (moved!)\n\n";
        std::cout << "  Contiguous storage means reallocation MOVES every element.\n";
        std::cout << "  All pointers, references, AND iterators are invalidated.\n";
        std::cout << "  This is the price of cache-friendly layout.\n\n";
    }

    // ---------------------------------------------------------------
    // Summary
    // ---------------------------------------------------------------
    std::cout << "=== Stability guarantees on insert ===\n\n";
    std::cout << "  Container              References/Ptrs    Iterators\n";
    std::cout << "  " << std::string(58, '-') << "\n";
    std::cout << "  std::map               STABLE             STABLE\n";
    std::cout << "  std::unordered_map     STABLE             invalidated on rehash\n";
    std::cout << "  flat_map / flat_hash   INVALIDATED        INVALIDATED\n\n";

    std::cout << "Case study: a real auth service keeps its in-flight request\n";
    std::cout << "map as std::map, not for ordering, but because the map grows\n";
    std::cout << "and shrinks under load. Stable per-op cost, no rehash spikes,\n";
    std::cout << "and reversal handlers can safely hold references.\n";

    return 0;
}
