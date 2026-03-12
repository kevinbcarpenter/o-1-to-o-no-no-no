// Section 5: C++23 flat_map - Acquirer Edition
// BIN table: loaded once at startup, queried millions of times per day
// This is the ideal flat_map use case.

#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

#if __has_include(<flat_map>)
#include <flat_map>
#define HAS_FLAT_MAP 1
#else
#define HAS_FLAT_MAP 0
#endif

#include <map>
#include <unordered_map>

struct BinInfo {
    std::string issuer;
    std::string network;
    std::string card_type;
};

template<typename Func>
double measure_ms(Func&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

int main() {
    std::cout << "=== BIN Table: The Perfect flat_map Use Case ===\n\n";

#if HAS_FLAT_MAP
    constexpr int BINS = 500'000;

    std::map<std::string, BinInfo> tree_map;
    std::flat_map<std::string, BinInfo> flat;
    std::unordered_map<std::string, BinInfo> hash_map;

    for (int i = 0; i < BINS; ++i) {
        char bin[9];
        snprintf(bin, sizeof(bin), "%08d", i);
        BinInfo info{"ISSUER_" + std::to_string(i % 500),
                     (i % 2 == 0) ? "VISA" : "MC",
                     (i % 3 == 0) ? "CREDIT" : "DEBIT"};
        tree_map[bin] = info;
        flat[bin] = info;
        hash_map[bin] = info;
    }

    volatile size_t sink = 0;

    double map_iter = measure_ms([&]() {
        for (const auto& [bin, info] : tree_map) sink += info.issuer.size();
    });

    double flat_iter = measure_ms([&]() {
        for (const auto& [bin, info] : flat) sink += info.issuer.size();
    });

    double hash_iter = measure_ms([&]() {
        for (const auto& [bin, info] : hash_map) sink += info.issuer.size();
    });

    std::cout << "Iteration over " << BINS << " BIN entries:\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  std::map:           " << map_iter << " ms\n";
    std::cout << "  std::flat_map:      " << flat_iter << " ms (cache-friendly!)\n";
    std::cout << "  std::unordered_map: " << hash_iter << " ms\n\n";

    std::cout << "BIN tables are loaded once, queried millions of times.\n";
    std::cout << "flat_map's contiguous storage makes iteration blazing fast.\n";

#else
    std::cout << "std::flat_map not available (requires C++23 library support)\n\n";
    std::cout << "Why flat_map is perfect for a BIN table:\n";
    std::cout << "  - BIN table is loaded at startup and rarely modified\n";
    std::cout << "  - O(n) insert cost is paid once, at boot\n";
    std::cout << "  - O(log n) lookup via binary search on contiguous memory\n";
    std::cout << "  - Cache-friendly iteration for batch reporting\n";
    std::cout << "  - No per-node allocation overhead\n\n";

    std::cout << "Usage:\n";
    std::cout << "  #include <flat_map>\n";
    std::cout << "  std::flat_map<std::string, BinInfo> bin_table;\n";
    std::cout << "  bin_table[\"41111100\"] = {\"CHASE\", \"VISA\", \"CREDIT\"};\n";
    std::cout << "  auto it = bin_table.find(\"41111100\");  // O(log n) binary search\n";
#endif

    return 0;
}
