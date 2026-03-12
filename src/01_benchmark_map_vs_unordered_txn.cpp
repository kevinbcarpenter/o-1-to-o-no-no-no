// Section 2: map vs unordered_map benchmark - Acquirer Edition
// Authorization lookup by auth_code: O(log n) vs O(1)

#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>
#include <string>

struct AuthRecord {
    std::string pan_masked;
    uint32_t amount_cents;
    std::string merchant_id;
    std::string response_code;
};

template<typename Func>
double measure_ns(Func&& f, int iterations = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

int main() {
    std::vector<size_t> sizes = {1'000, 10'000, 100'000, 1'000'000, 10'000'000};

    std::random_device rd;
    std::mt19937_64 gen(rd());

    std::cout << "=== Acquirer Auth Lookup: map vs unordered_map ===\n\n";
    std::cout << std::setw(12) << "Auths"
              << std::setw(20) << "std::map (ns)"
              << std::setw(20) << "unordered_map (ns)"
              << std::setw(15) << "Speedup"
              << "\n" << std::string(67, '-') << "\n";

    for (auto n : sizes) {
        std::map<std::string, AuthRecord> ordered;
        std::unordered_map<std::string, AuthRecord> unordered;

        std::uniform_int_distribution<uint32_t> amount_dist(100, 999999);
        std::vector<std::string> auth_codes(n);

        for (size_t i = 0; i < n; ++i) {
            char buf[16];
            snprintf(buf, sizeof(buf), "AUTH%08zu", i);
            auth_codes[i] = buf;

            AuthRecord rec{"411111******1111", amount_dist(gen), "MERCH_001", "00"};
            ordered[auth_codes[i]] = rec;
            unordered[auth_codes[i]] = rec;
        }

        std::uniform_int_distribution<size_t> idx_dist(0, n - 1);
        std::vector<std::string> lookup_keys(1000);
        for (auto& k : lookup_keys) k = auth_codes[idx_dist(gen)];

        volatile uint32_t sink = 0;
        size_t idx = 0;

        double map_time = measure_ns([&]() {
            sink = ordered.find(lookup_keys[idx++ % 1000])->second.amount_cents;
        }, 10000);

        idx = 0;
        double unordered_time = measure_ns([&]() {
            sink = unordered.find(lookup_keys[idx++ % 1000])->second.amount_cents;
        }, 10000);

        std::cout << std::setw(12) << n
                  << std::setw(20) << std::fixed << std::setprecision(1) << map_time
                  << std::setw(20) << unordered_time
                  << std::setw(14) << std::setprecision(1) << (map_time / unordered_time) << "x"
                  << "\n";
    }

    std::cout << "\nAt scale, every nanosecond matters in auth processing.\n";

    return 0;
}
