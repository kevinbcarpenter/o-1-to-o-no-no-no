// Section 4, Pitfall 2: Collision Catastrophe - Acquirer Edition
// What if every auth request hashes to the same bucket?

#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

// Simulates a terrible hash: all auth codes collide
struct TerribleAuthHash {
    size_t operator()(const std::string&) const {
        return 42;  // Every auth code goes to bucket 42
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
    std::cout << "=== Collision Catastrophe: Auth Lookup Meltdown ===\n\n";

    std::cout << std::setw(10) << "Auths"
              << std::setw(18) << "std::map"
              << std::setw(18) << "unordered(good)"
              << std::setw(18) << "unordered(bad)"
              << "\n" << std::string(64, '-') << "\n";

    for (int n : {100, 500, 1000, 2000, 5000}) {
        std::map<std::string, uint32_t> ordered;
        std::unordered_map<std::string, uint32_t> good_hash;
        std::unordered_map<std::string, uint32_t, TerribleAuthHash> bad_hash;

        std::vector<std::string> auth_codes;
        auth_codes.reserve(n);

        for (int i = 0; i < n; ++i) {
            std::string auth = "AUTH" + std::to_string(100000 + i);
            auth_codes.push_back(auth);
            ordered[auth] = i * 100;
            good_hash[auth] = i * 100;
            bad_hash[auth] = i * 100;
        }

        double map_ms = benchmark_lookups(ordered, auth_codes);
        double good_ms = benchmark_lookups(good_hash, auth_codes);
        double bad_ms = benchmark_lookups(bad_hash, auth_codes);

        std::cout << std::setw(10) << n
                  << std::setw(15) << std::fixed << std::setprecision(2) << map_ms << " ms"
                  << std::setw(15) << good_ms << " ms"
                  << std::setw(15) << bad_ms << " ms"
                  << "\n";
    }

    std::cout << "\nWith a bad hash, auth lookups degrade to O(n).\n";
    std::cout << "At 5000 in-flight auths, that's a compliance-level latency breach.\n";

    return 0;
}
