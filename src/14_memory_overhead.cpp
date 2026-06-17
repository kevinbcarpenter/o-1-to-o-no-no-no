// Section 8: Memory Deep Dive - Acquirer Edition
// MEASURE per-element overhead instead of quoting numbers off a slide.
// A counting allocator tallies real heap bytes for map vs unordered_map vs a
// contiguous (flat) vector, on whatever stdlib you build with.

#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <cstddef>

static size_t g_bytes = 0;
static size_t g_peak  = 0;

template <typename T>
struct CountingAlloc {
    using value_type = T;
    CountingAlloc() = default;
    template <typename U> CountingAlloc(const CountingAlloc<U>&) {}
    T* allocate(size_t n) {
        size_t b = n * sizeof(T);
        g_bytes += b; g_peak = std::max(g_peak, g_bytes);
        return static_cast<T*>(::operator new(b));
    }
    void deallocate(T* p, size_t n) {
        g_bytes -= n * sizeof(T);
        ::operator delete(p);
    }
    template <typename U> bool operator==(const CountingAlloc<U>&) const { return true; }
    template <typename U> bool operator!=(const CountingAlloc<U>&) const { return false; }
};

int main() {
    constexpr int N = 1'000'000;
    using K = uint64_t;        // fixed-size keys to isolate container overhead
    using V = uint64_t;

    std::cout << "=== Per-element memory overhead (measured) ===\n";
    std::cout << "N = " << N << ", key=value=uint64_t (8 bytes each, 16 data bytes/elem)\n\n";

    auto report = [&](const char* name, size_t peak) {
        double per = double(peak) / N;
        std::cout << "  " << std::left << std::setw(22) << name
                  << std::right << std::setw(12) << peak << " bytes  "
                  << std::fixed << std::setprecision(1) << per << " bytes/elem  ("
                  << (per - 16.0) << " overhead)\n";
    };

    { // std::map (red-black tree node per element)
        g_bytes = g_peak = 0;
        std::map<K, V, std::less<K>, CountingAlloc<std::pair<const K, V>>> m;
        for (int i = 0; i < N; ++i) m[i] = i;
        report("std::map", g_peak);
    }
    { // std::unordered_map (node per element + bucket array)
        g_bytes = g_peak = 0;
        std::unordered_map<K, V, std::hash<K>, std::equal_to<K>,
                           CountingAlloc<std::pair<const K, V>>> m;
        for (int i = 0; i < N; ++i) m[i] = i;
        report("std::unordered_map", g_peak);
    }
    { // contiguous flat layout (what flat_map / flat hash maps approach)
        g_bytes = g_peak = 0;
        std::vector<std::pair<K, V>, CountingAlloc<std::pair<K, V>>> v;
        v.reserve(N);
        for (int i = 0; i < N; ++i) v.emplace_back(i, i);
        report("flat vector<pair>", g_peak);
    }

    std::cout << "\nNode-based containers pay for pointers + per-node allocation headers.\n";
    std::cout << "Contiguous storage pays ~0 overhead. Numbers vary by stdlib, so measure yours.\n";
    return 0;
}
