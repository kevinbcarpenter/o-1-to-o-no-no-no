// Round 2: The container is usually not your bottleneck - Acquirer Edition
// Distilled from a real auth service: a read-mostly lookup table is wrapped in
// a SharedResource and unlocked PER lookup, inside per-request loops, on MANY
// worker threads at once. Single-threaded, the lock is cheap and the container
// wins. Under concurrency, the per-lookup shared_lock causes cache-line
// contention that collapses throughput and swamps the map-vs-unordered choice.
// The real win is hoisting the lock out of the loop.
//
// Patterns drawn from a real payment-auth service; code here is distilled.

#include <iostream>
#include <map>
#include <unordered_map>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>

// Minimal stand-in for the service's company::SharedResource<T>:
// getShared() returns a handle holding a shared (reader) lock for its life.
template <typename T>
class SharedResource {
    T value_;
    mutable std::shared_mutex mu_;
public:
    template <typename... A>
    explicit SharedResource(A&&... a) : value_(std::forward<A>(a)...) {}

    class SharedHandle {
        const T* p_;
        std::shared_lock<std::shared_mutex> lk_;
    public:
        SharedHandle(const T* p, std::shared_mutex& m) : p_(p), lk_(m) {}
        const T* operator->() const { return p_; }
        const T& operator*() const { return *p_; }
    };
    SharedHandle getShared() const { return SharedHandle(&value_, mu_); }
};

// Returns wall-clock ms for `threads` workers doing `perThread` lookups each.
// per_lock=true  -> getShared() inside the loop (every lookup re-locks)
// per_lock=false -> getShared() once per thread, then loop (hoisted)
template <typename Map>
double run(const std::vector<std::string>& keys, int threads, int perThread, bool per_lock) {
    Map raw;
    for (size_t i = 0; i < keys.size(); ++i) raw[keys[i]] = static_cast<int>(i);
    SharedResource<Map> table{std::move(raw)};

    std::atomic<long> sink{0};
    std::atomic<bool> go{false};

    auto worker = [&](int seed) {
        std::mt19937_64 gen(seed);
        std::uniform_int_distribution<size_t> pick(0, keys.size() - 1);
        std::vector<size_t> idx(perThread);
        for (auto& x : idx) x = pick(gen);

        while (!go.load(std::memory_order_acquire)) { /* spin to start together */ }

        long local = 0;
        if (per_lock) {
            for (int i = 0; i < perThread; ++i) {
                auto h = table.getShared();                 // lock every lookup
                auto it = h->find(keys[idx[i]]);
                if (it != h->end()) local += it->second;
            }
        } else {
            auto h = table.getShared();                     // lock once
            for (int i = 0; i < perThread; ++i) {
                auto it = h->find(keys[idx[i]]);
                if (it != h->end()) local += it->second;
            }
        }
        sink += local;
    };

    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker, t + 1);
    auto start = std::chrono::high_resolution_clock::now();
    go.store(true, std::memory_order_release);
    for (auto& th : pool) th.join();
    auto end = std::chrono::high_resolution_clock::now();
    (void)sink;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    const int threads = std::max(4u, std::thread::hardware_concurrency());
    constexpr int PER_THREAD = 1'000'000;

    std::cout << "=== Concurrent read-mostly table: where does the time go? ===\n";
    std::cout << threads << " worker threads, " << PER_THREAD << " lookups each\n\n";

    // ~100-entry table with realistic, longer-than-SSO field keys
    std::vector<std::string> keys;
    for (int i = 0; i < 100; ++i) {
        char b[32];
        snprintf(b, sizeof(b), "FIELD_VALIDATION_TAG_%05d", i);
        keys.emplace_back(b);
    }

    double map_lock   = run<std::map<std::string, int>>(keys, threads, PER_THREAD, true);
    double umap_lock  = run<std::unordered_map<std::string, int>>(keys, threads, PER_THREAD, true);
    double map_hoist  = run<std::map<std::string, int>>(keys, threads, PER_THREAD, false);
    double umap_hoist = run<std::unordered_map<std::string, int>>(keys, threads, PER_THREAD, false);

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "getShared() PER lookup (contended):\n";
    std::cout << "  std::map           : " << std::setw(8) << map_lock  << " ms\n";
    std::cout << "  std::unordered_map : " << std::setw(8) << umap_lock << " ms"
              << "   <- container barely matters; everyone is lock-bound\n\n";
    std::cout << "getShared() hoisted (lock once per thread):\n";
    std::cout << "  std::map           : " << std::setw(8) << map_hoist  << " ms\n";
    std::cout << "  std::unordered_map : " << std::setw(8) << umap_hoist << " ms\n\n";

    std::cout << "Hoisting the lock: " << (map_lock / map_hoist) << "x faster (map), "
              << (umap_lock / umap_hoist) << "x faster (unordered).\n";
    std::cout << "Under real concurrency the per-lookup shared_lock dominated.\n";
    std::cout << "Lesson: profile first. Hoisting the lock beat swapping the container.\n";
    return 0;
}
