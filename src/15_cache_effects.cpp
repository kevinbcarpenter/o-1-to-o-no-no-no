// Section 8: Cache Effects - Acquirer Edition
// "O(1)" lookup latency is not constant in practice: as the working set spills
// out of L1 -> L2 -> L3 -> RAM, each lookup costs more. This is where the flat
// line bends. Compare scattered (unordered_map) vs contiguous (sorted vector).

#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstdint>
#include <iomanip>

int main() {
    std::cout << "=== Cache cliff: lookup latency vs working-set size ===\n\n";
    std::cout << std::setw(12) << "Entries"
              << std::setw(14) << "~Data (KB)"
              << std::setw(18) << "unordered (ns)"
              << std::setw(18) << "sorted vec (ns)" << "\n";
    std::cout << std::string(62, '-') << "\n";

    std::mt19937_64 gen(7);

    for (size_t n : {size_t(1<<8), size_t(1<<10), size_t(1<<12), size_t(1<<14),
                     size_t(1<<16), size_t(1<<18), size_t(1<<20), size_t(1<<22)}) {
        std::unordered_map<uint64_t, uint64_t> umap;
        umap.reserve(n);
        std::vector<std::pair<uint64_t, uint64_t>> sorted;
        sorted.reserve(n);
        for (uint64_t i = 0; i < n; ++i) { umap[i] = i; sorted.emplace_back(i, i); }
        std::sort(sorted.begin(), sorted.end());

        std::uniform_int_distribution<uint64_t> pick(0, n - 1);
        std::vector<uint64_t> probes(1u << 20);
        for (auto& p : probes) p = pick(gen);

        volatile uint64_t sink = 0;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (auto p : probes) { auto it = umap.find(p); if (it != umap.end()) sink += it->second; }
        auto t1 = std::chrono::high_resolution_clock::now();
        for (auto p : probes) {
            auto it = std::lower_bound(sorted.begin(), sorted.end(), p,
                        [](auto& kv, uint64_t k){ return kv.first < k; });
            if (it != sorted.end() && it->first == p) sink += it->second;
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        double u_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / probes.size();
        double s_ns = std::chrono::duration<double, std::nano>(t2 - t1).count() / probes.size();
        double kb = double(n * 2 * sizeof(uint64_t)) / 1024.0;

        std::cout << std::setw(12) << n
                  << std::setw(14) << std::fixed << std::setprecision(0) << kb
                  << std::setw(18) << std::setprecision(1) << u_ns
                  << std::setw(18) << s_ns
                  << (sink == 42 ? " " : "") << "\n";
    }
    std::cout << "\nWatch unordered_map's per-lookup cost climb as the table outgrows cache.\n";
    std::cout << "O(1) in element count, but the constant is a cache-miss in disguise.\n";
    return 0;
}
