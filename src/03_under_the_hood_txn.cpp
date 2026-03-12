// Section 3: Under the Hood - Acquirer Edition
// Bucket structure with merchant ID -> merchant name mapping

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <iomanip>

void print_bucket_info(const std::unordered_map<std::string, std::string>& u_map) {
    std::cout << "Size: " << u_map.size() << "\n";
    std::cout << "Bucket count: " << u_map.bucket_count() << "\n";
    std::cout << "Load factor: " << std::fixed << std::setprecision(2) << u_map.load_factor() << "\n";
    std::cout << "Max load factor: " << u_map.max_load_factor() << "\n\n";

    std::cout << "Bucket distribution:\n";
    std::cout << std::string(60, '-') << "\n";

    for (size_t i = 0; i < u_map.bucket_count(); ++i) {
        size_t bucket_size = u_map.bucket_size(i);
        if (bucket_size > 0) {
            std::cout << "Bucket " << std::setw(3) << i << " [" << bucket_size << "]: ";
            for (auto it = u_map.begin(i); it != u_map.end(i); ++it) {
                std::cout << "\"" << it->first << "\" ";
            }
            std::cout << "\n";
        }
    }
}

int main() {
    std::unordered_map<std::string, std::string> merchant_cache;

    std::cout << "=== Merchant Cache: Bucket Layout ===\n\n";
    std::cout << "Adding merchants one by one...\n\n";

    std::vector<std::pair<std::string, std::string>> merchants = {
        {"MID_100001", "AMAZON WEB SERVICES"},
        {"MID_200045", "WALMART STORE 3291"},
        {"MID_300078", "SHELL GAS STATION"},
        {"MID_400012", "STARBUCKS CORP"},
        {"MID_500099", "UNITED AIRLINES"},
        {"MID_600023", "TARGET STORE 1482"}
    };

    for (const auto& [mid, name] : merchants) {
        merchant_cache[mid] = name;

        size_t hash = std::hash<std::string>{}(mid);
        size_t bucket = hash % merchant_cache.bucket_count();

        std::cout << "Inserted \"" << mid << "\" -> \"" << name << "\"\n";
        std::cout << "  hash(\"" << mid << "\") = " << hash << "\n";
        std::cout << "  bucket = " << hash << " % "
                  << merchant_cache.bucket_count() << " = " << bucket << "\n\n";
    }

    std::cout << "\n=== Final bucket layout ===\n\n";
    print_bucket_info(merchant_cache);

    return 0;
}
