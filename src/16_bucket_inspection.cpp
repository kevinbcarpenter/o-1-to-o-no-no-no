// Section 9: Profiling - Acquirer Edition
// You suspect a hot bucket in production. Here's how to SEE it at runtime,
// using only the standard bucket interface. Detect a bad hash before it pages you.

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iomanip>

// A plausibly-bad hash: clusters auth codes by their last two digits.
struct ClusteringHash {
    size_t operator()(const std::string& s) const {
        return s.size() >= 2 ? (s[s.size()-1] * 10 + s[s.size()-2]) : 0;
    }
};

template <typename Map>
void inspect(const char* label, const Map& m) {
    size_t buckets = m.bucket_count();
    size_t occupied = 0, max_size = 0;
    double mean = double(m.size()) / buckets;
    std::vector<size_t> sizes;
    for (size_t i = 0; i < buckets; ++i) {
        size_t bs = m.bucket_size(i);
        if (bs) { ++occupied; sizes.push_back(bs); }
        max_size = std::max(max_size, bs);
    }
    std::cout << "--- " << label << " ---\n";
    std::cout << "  size=" << m.size() << " buckets=" << buckets
              << " load_factor=" << std::fixed << std::setprecision(2) << m.load_factor() << "\n";
    std::cout << "  occupied buckets=" << occupied
              << "  mean/occupied=" << (occupied ? double(m.size())/occupied : 0)
              << "  max bucket=" << max_size << " (ideal ~" << std::setprecision(1) << mean << ")\n";

    // Flag hot buckets: > 8x the mean is a red flag worth logging in prod.
    size_t threshold = std::max<size_t>(8, size_t(mean * 8) + 1);
    size_t hot = std::count_if(sizes.begin(), sizes.end(),
                               [&](size_t s){ return s > threshold; });
    std::cout << "  hot buckets (> " << threshold << " entries): " << hot
              << (hot ? "  <-- investigate the hash!\n" : "\n") << "\n";
}

int main() {
    std::cout << "=== Runtime bucket-occupancy inspection ===\n\n";
    const int N = 50000;

    std::unordered_map<std::string, uint32_t> good;
    std::unordered_map<std::string, uint32_t, ClusteringHash> bad;
    good.reserve(N);
    for (int i = 0; i < N; ++i) {
        std::string code = "AUTH" + std::to_string(100000 + i);
        good[code] = i; bad[code] = i;
    }
    inspect("default std::hash", good);
    inspect("clustering hash", bad);

    std::cout << "Drop this loop behind a /debug endpoint or a periodic metric.\n";
    std::cout << "A max bucket many times the mean = a hash problem, not a load problem.\n";
    return 0;
}
