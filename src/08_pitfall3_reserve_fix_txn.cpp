// Section 4, Pitfall 3: The Fix - Acquirer Edition
// reserve() for known batch sizes eliminates rehash spikes

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <string>

constexpr int TPS = 150;
constexpr int TOTAL_AUTHORIZATIONS = TPS * 60 * 60;

struct InsertStats {
    double avg_us;
    double max_us;
    int rehash_count;
};

InsertStats benchmark_inserts(bool use_reserve, int n) {
    std::unordered_map<std::string, uint32_t> auth_cache;

    if (use_reserve) {
        auth_cache.reserve(n);
    }

    std::vector<double> times;
    times.reserve(n);
    size_t prev_buckets = auth_cache.bucket_count();
    int rehashes = 0;

    for (int i = 0; i < n; ++i) {
        std::string auth_code = "AUTH" + std::to_string(100000 + i);

        auto start = std::chrono::high_resolution_clock::now();
        auth_cache[auth_code] = 1000 + i;
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        times.push_back(std::chrono::duration<double, std::micro>(elapsed).count());

        if (auth_cache.bucket_count() != prev_buckets) {
            ++rehashes;
            prev_buckets = auth_cache.bucket_count();
        }
    }

    double sum = 0;
    for (double t : times) sum += t;

    return {
        sum / times.size(),
        *std::max_element(times.begin(), times.end()),
        rehashes
    };
}

int main() {
    std::cout << "=== The Fix: reserve() for Auth Batch Processing ===\n\n";

    std::cout << "Processing " << TPS << " TPS over 60 minutes (" << TOTAL_AUTHORIZATIONS << " authorizations)...\n\n";

    auto no_reserve = benchmark_inserts(false, TOTAL_AUTHORIZATIONS);
    auto with_reserve = benchmark_inserts(true, TOTAL_AUTHORIZATIONS);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "                    Without reserve()    With reserve()\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Avg insert time:    " << std::setw(12) << no_reserve.avg_us << " us"
              << std::setw(16) << with_reserve.avg_us << " us\n";
    std::cout << "Max insert time:    " << std::setw(12) << no_reserve.max_us << " us"
              << std::setw(16) << with_reserve.max_us << " us\n";
    std::cout << "Rehash events:      " << std::setw(12) << no_reserve.rehash_count
              << std::setw(19) << with_reserve.rehash_count << "\n";

    std::cout << "\nMax spike reduction: "
              << (no_reserve.max_us / with_reserve.max_us) << "x\n";

    std::cout << "\nYou know your daily auth volume. Call reserve()!\n";

    return 0;
}
