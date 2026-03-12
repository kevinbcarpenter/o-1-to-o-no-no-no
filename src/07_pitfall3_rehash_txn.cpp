// Section 4, Pitfall 3: Hidden Rehash - Acquirer Edition
// Latency spikes in the auth stream from rehashing

#include <iostream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

constexpr int TPS = 150;
constexpr int TOTAL_AUTHORIZATIONS = TPS * 60 * 60;

int main() {
    std::cout << "=== Rehash Latency Spikes in Auth Processing ===\n\n";

    std::unordered_map<std::string, uint32_t> auth_cache;

    std::cout << "Simulating " << TPS << " TPS over 60 minutes (" << TOTAL_AUTHORIZATIONS << " authorizations)...\n\n";
    std::cout << std::setw(12) << "Auth #"
              << std::setw(15) << "Time (us)"
              << std::setw(15) << "Buckets"
              << std::setw(15) << "Load Factor"
              << "  Event\n";
    std::cout << std::string(72, '-') << "\n";

    size_t prev_buckets = 0;
    std::vector<std::pair<int, double>> spikes;

    for (int i = 0; i < TOTAL_AUTHORIZATIONS; ++i) {
        std::string auth_code = "AUTH" + std::to_string(100000 + i);

        auto start = std::chrono::high_resolution_clock::now();
        auth_cache[auth_code] = 1000 + (i % 99900);
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        double us = std::chrono::duration<double, std::micro>(elapsed).count();

        if (auth_cache.bucket_count() != prev_buckets) {
            std::cout << std::setw(12) << i
                      << std::setw(15) << std::fixed << std::setprecision(1) << us
                      << std::setw(15) << auth_cache.bucket_count()
                      << std::setw(15) << std::setprecision(2) << auth_cache.load_factor()
                      << "  ** REHASH **\n";
            spikes.push_back({i, us});
            prev_buckets = auth_cache.bucket_count();
        }
    }

    std::cout << "\nRehash events: " << spikes.size() << "\n";
    std::cout << "Final auths cached: " << auth_cache.size() << "\n";
    std::cout << "Final buckets: " << auth_cache.bucket_count() << "\n";
    std::cout << "\nEach rehash is a stop-the-world event.\n";
    std::cout << "In auth processing, these spikes violate SLA latency targets.\n";

    return 0;
}
