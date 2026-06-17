// Section 9: Profiling workload - Acquirer Edition
// A clean, single-purpose binary to run under a profiler so the cache-miss
// counters reflect the MAP, not setup noise. Minimal output.
//
//   Linux (in Docker):   perf stat -e cache-misses,L1-dcache-load-misses ./19_perf_workload chained
//   macOS:               run under Instruments "Counters", or mperf -- ./19_perf_workload flat
//
// arg1: "chained" (std::unordered_map) | "flat" (contiguous sorted vector)

#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <random>
#include <string>
#include <cstdint>

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "chained";
    constexpr int N = 4'000'000;
    constexpr int LOOKUPS = 20'000'000;

    std::mt19937_64 gen(99);
    std::uniform_int_distribution<uint64_t> pick(0, N - 1);

    std::vector<uint64_t> probes(LOOKUPS);
    for (auto& p : probes) p = pick(gen);

    volatile uint64_t sink = 0;

    if (mode == "flat") {
        std::vector<std::pair<uint64_t, uint64_t>> v;
        v.reserve(N);
        for (uint64_t i = 0; i < N; ++i) v.emplace_back(i, i * 3);
        // already sorted by construction
        for (auto p : probes) {
            auto it = std::lower_bound(v.begin(), v.end(), p,
                        [](auto& kv, uint64_t k){ return kv.first < k; });
            if (it != v.end() && it->first == p) sink += it->second;
        }
    } else {
        std::unordered_map<uint64_t, uint64_t> m;
        m.reserve(N);
        for (uint64_t i = 0; i < N; ++i) m[i] = i * 3;
        for (auto p : probes) {
            auto it = m.find(p);
            if (it != m.end()) sink += it->second;
        }
    }

    std::cout << mode << ": " << LOOKUPS << " lookups over " << N
              << " entries, checksum=" << (sink % 1000003) << "\n";
    return 0;
}
