// Section 4, Pitfall 2: Collision Catastrophe - Acquirer Edition
// Hashing on card_type alone: only 4 values (VISA, MC, AMEX, DISCOVER).
// 5,000 auths across 4 buckets = ~1,250 per chain. Every lookup is O(n).

#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

static const char* CARD_TYPES[] = {"VISA", "MC", "AMEX", "DISCOVER"};

std::string get_card_type(const std::string& auth_code) {
    // Simulate: derive card type from the auth code's trailing digit
    int idx = (auth_code.back() - '0') % 4;
    return CARD_TYPES[idx];
}

// Hashes only on card type. 4 distinct values, so 4 buckets carry everything.
struct CardTypeHash {
    size_t operator()(const std::string& auth) const {
        return std::hash<std::string>{}(get_card_type(auth));
    }
};

template<typename Map>
double benchmark_lookups(Map& m, const std::vector<std::string>& keys) {
    auto start = std::chrono::high_resolution_clock::now();
    for (const auto& k : keys) {
        volatile auto it = m.find(k);
    }
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

int main() {
    std::cout << "=== Collision Catastrophe: Hashing on Card Type Alone ===\n\n";
    std::cout << "Only 4 card types (VISA, MC, AMEX, DISCOVER).\n";
    std::cout << "All auths land in one of 4 buckets.\n\n";

    std::cout << std::setw(10) << "Auths"
              << std::setw(18) << "std::map"
              << std::setw(18) << "unordered(good)"
              << std::setw(18) << "unordered(bad)"
              << "\n" << std::string(64, '-') << "\n";

    for (int n : {100, 500, 1000, 2000, 5000}) {
        std::map<std::string, uint32_t> ordered;
        std::unordered_map<std::string, uint32_t> good_hash;
        std::unordered_map<std::string, uint32_t, CardTypeHash> bad_hash;

        std::vector<std::string> auth_codes;
        auth_codes.reserve(n);

        for (int i = 0; i < n; ++i) {
            std::string auth = "AUTH" + std::to_string(100000 + i);
            auth_codes.push_back(auth);
            ordered[auth] = i * 100;
            good_hash[auth] = i * 100;
            bad_hash[auth] = i * 100;
        }

        // Show the damage
        size_t max_bucket = 0;
        for (size_t b = 0; b < bad_hash.bucket_count(); ++b)
            max_bucket = std::max(max_bucket, bad_hash.bucket_size(b));

        double map_ms = benchmark_lookups(ordered, auth_codes);
        double good_ms = benchmark_lookups(good_hash, auth_codes);
        double bad_ms = benchmark_lookups(bad_hash, auth_codes);

        std::cout << std::setw(10) << n
                  << std::setw(15) << std::fixed << std::setprecision(2) << map_ms << " ms"
                  << std::setw(15) << good_ms << " ms"
                  << std::setw(15) << bad_ms << " ms";
        if (n == 5000)
            std::cout << "   (worst bucket: " << max_bucket << ")";
        std::cout << "\n";
    }

    std::cout << "\nWith only 4 card types, each bucket holds ~n/4 auths.\n";
    std::cout << "Every lookup walks a chain of ~1,250 nodes at n=5000.\n";
    std::cout << "Same mistake as Pitfall 1 (currency), fewer distinct values, worse outcome.\n";

    return 0;
}
