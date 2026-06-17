// Section 9: Multithreading - Acquirer Edition
// std::unordered_map is NOT safe for concurrent read+write. A single global
// mutex serializes everything. Sharding (N maps, each with its own lock) lets
// independent keys proceed in parallel. Demo: throughput, single-lock vs sharded.

#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <cstdint>
#include <iomanip>

// One big map behind one mutex.
struct SingleLockMap {
    std::unordered_map<uint64_t, uint64_t> m;
    std::mutex mu;
    void upsert(uint64_t k, uint64_t v) { std::lock_guard<std::mutex> g(mu); m[k] = v; }
    bool get(uint64_t k, uint64_t& out) {
        std::lock_guard<std::mutex> g(mu);
        auto it = m.find(k); if (it == m.end()) return false; out = it->second; return true;
    }
};

// N shards: key % N picks the shard, so unrelated keys rarely contend.
struct ShardedMap {
    struct Shard { std::unordered_map<uint64_t, uint64_t> m; std::mutex mu; };
    std::vector<Shard> shards;
    explicit ShardedMap(size_t n) : shards(n) {}
    Shard& shard_for(uint64_t k) { return shards[k % shards.size()]; }
    void upsert(uint64_t k, uint64_t v) {
        auto& s = shard_for(k); std::lock_guard<std::mutex> g(s.mu); s.m[k] = v;
    }
    bool get(uint64_t k, uint64_t& out) {
        auto& s = shard_for(k); std::lock_guard<std::mutex> g(s.mu);
        auto it = s.m.find(k); if (it == s.m.end()) return false; out = it->second; return true;
    }
};

template <typename Map>
double run(Map& map, int threads, int ops_per_thread) {
    std::atomic<uint64_t> done{0};
    auto t0 = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t] {
            uint64_t local = 0;
            for (int i = 0; i < ops_per_thread; ++i) {
                uint64_t k = (uint64_t(t) << 20) ^ (i * 2654435761u);
                if (i % 4 == 0) map.upsert(k, i);
                else { uint64_t v; if (map.get(k, v)) local += v; }
            }
            done += local;
        });
    }
    for (auto& th : pool) th.join();
    auto t1 = std::chrono::high_resolution_clock::now();
    (void)done;
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    const int threads = std::max(2u, std::thread::hardware_concurrency());
    const int ops = 500000;
    std::cout << "=== Concurrent auth cache: single lock vs sharded ===\n";
    std::cout << "threads=" << threads << ", ops/thread=" << ops
              << " (75% reads, 25% writes)\n\n";

    SingleLockMap single;
    ShardedMap sharded(threads * 4);

    double single_ms = run(single, threads, ops);
    double sharded_ms = run(sharded, threads, ops);

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  single mutex: " << single_ms << " ms\n";
    std::cout << "  sharded (" << threads * 4 << " shards): " << sharded_ms << " ms\n";
    std::cout << "  speedup: " << std::setprecision(2) << (single_ms / sharded_ms) << "x\n\n";
    std::cout << "Sharding trades a little memory for parallelism. For real workloads,\n";
    std::cout << "reach for tbb::concurrent_hash_map or folly::ConcurrentHashMap.\n";
    return 0;
}
