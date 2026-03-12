// Section 2: Simplified benchmark for slides - Acquirer Edition
// Transaction ID lookup - clean version for slides

#include <iostream>
#include <map>
#include <unordered_map>
#include <chrono>
#include <string>

using namespace std::chrono;

int main() {
    constexpr int N = 1'000'000;

    // Simulate an auth cache keyed by transaction ID
    std::map<std::string, uint32_t> ordered;
    std::unordered_map<std::string, uint32_t> unordered;

    for (int i = 0; i < N; ++i) {
        std::string txn_id = "TXN" + std::to_string(i);
        ordered[txn_id] = i * 100;   // amount in cents
        unordered[txn_id] = i * 100;
    }

    // Benchmark std::map lookup
    auto start = high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        std::string key = "TXN" + std::to_string(i);
        volatile auto it = ordered.find(key);
    }
    auto map_time = high_resolution_clock::now() - start;

    // Benchmark std::unordered_map lookup
    start = high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        std::string key = "TXN" + std::to_string(i);
        volatile auto it = unordered.find(key);
    }
    auto unordered_time = high_resolution_clock::now() - start;

    std::cout << "=== Auth Cache Lookup: " << N << " transactions ===\n\n";
    std::cout << "std::map:           " << duration_cast<milliseconds>(map_time).count() << " ms\n";
    std::cout << "std::unordered_map: " << duration_cast<milliseconds>(unordered_time).count() << " ms\n";
    std::cout << "Speedup: " << (double)map_time.count() / unordered_time.count() << "x\n";

    return 0;
}
