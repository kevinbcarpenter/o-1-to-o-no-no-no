// Section 7: Open Addressing & Faster Maps - Acquirer Edition
// Why std::unordered_map (chaining) loses to a flat hash map on cache-bound work.
//
// Uses boost::unordered_flat_map when available (real-world SwissTable-style map).
// Falls back to a tiny hand-rolled linear-probing table so the comparison still
// runs without Boost. Build on GCC 15 + Homebrew Boost for the headline numbers.

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <random>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>

#if __has_include(<boost/unordered/unordered_flat_map.hpp>)
#include <boost/unordered/unordered_flat_map.hpp>
#define HAS_BOOST_FLAT 1
#else
#define HAS_BOOST_FLAT 0
#endif

// Minimal open-addressing (linear probing) map, used only as the no-Boost
// fallback. Insert + find, no erase. Illustrative, not production code.
template <typename V>
class FlatProbeMap {
    struct Slot { std::string key; V val; bool used = false; };
    std::vector<Slot> slots_;
    size_t count_ = 0;
    static size_t h(const std::string& k) { return std::hash<std::string>{}(k); }
    void grow() {
        std::vector<Slot> old = std::move(slots_);
        slots_.assign(old.empty() ? 16 : old.size() * 2, Slot{});
        count_ = 0;
        for (auto& s : old) if (s.used) insert(s.key, s.val);
    }
public:
    FlatProbeMap() : slots_(16) {}
    void insert(const std::string& k, const V& v) {
        if ((count_ + 1) * 10 >= slots_.size() * 7) grow();  // ~0.7 load factor
        size_t i = h(k) & (slots_.size() - 1);
        while (slots_[i].used && slots_[i].key != k) i = (i + 1) & (slots_.size() - 1);
        if (!slots_[i].used) { slots_[i].used = true; ++count_; }
        slots_[i].key = k; slots_[i].val = v;
    }
    const V* find(const std::string& k) const {
        size_t i = h(k) & (slots_.size() - 1);
        while (slots_[i].used) {
            if (slots_[i].key == k) return &slots_[i].val;
            i = (i + 1) & (slots_.size() - 1);
        }
        return nullptr;
    }
    template <typename F> void for_each(F f) const {
        for (auto& s : slots_) if (s.used) f(s.key, s.val);
    }
};

template <typename Func>
double measure_ms(Func&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    return std::chrono::duration<double, std::milli>(
        std::chrono::high_resolution_clock::now() - start).count();
}

int main() {
    constexpr int N = 1'000'000;
    std::cout << "=== Chaining vs Flat Hash Map: BIN Lookup ("
              << N << " entries) ===\n\n";
#if HAS_BOOST_FLAT
    std::cout << "Flat map: boost::unordered_flat_map (real SwissTable-style)\n\n";
    boost::unordered_flat_map<std::string, uint32_t> flat;
#else
    std::cout << "Flat map: hand-rolled linear-probing fallback "
                 "(install Boost for real numbers)\n\n";
    FlatProbeMap<uint32_t> flat;
#endif
    std::unordered_map<std::string, uint32_t> chained;

    std::vector<std::string> keys;
    keys.reserve(N);
    for (int i = 0; i < N; ++i) {
        char b[16]; snprintf(b, sizeof(b), "BIN%09d", i);
        keys.emplace_back(b);
    }

    chained.reserve(N);
    for (int i = 0; i < N; ++i) {
        chained[keys[i]] = i;
        flat.insert(keys[i], i);
    }

    std::mt19937_64 gen(123);
    std::uniform_int_distribution<int> pick(0, N - 1);
    std::vector<std::string> probes(200000);
    for (auto& p : probes) p = keys[pick(gen)];

    volatile uint64_t sink = 0;
    double chained_ms = measure_ms([&]{
        for (auto& p : probes) { auto it = chained.find(p); if (it != chained.end()) sink += it->second; }
    });
    double flat_ms = measure_ms([&]{
        for (auto& p : probes) { auto* v = flat.find(p); if (v) sink += *v; }
    });

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Random lookups (" << probes.size() << "):\n";
    std::cout << "  std::unordered_map (chaining): " << chained_ms << " ms\n";
    std::cout << "  flat hash map:                 " << flat_ms << " ms\n";
    std::cout << "  speedup: " << (chained_ms / flat_ms) << "x\n\n";

    std::cout << "The flat map keeps keys in one contiguous array: fewer cache misses.\n";
    std::cout << "Trade-off: it invalidates references on ANY insert (like a constant rehash).\n";
    std::cout << "(void)sink: " << (sink ? 1 : 0) << "\n";
    return 0;
}
