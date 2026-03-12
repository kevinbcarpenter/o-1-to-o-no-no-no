// Section 5: Heterogeneous Lookup - Acquirer Edition
// Look up merchants by string_view from incoming ISO 8583 messages
// without allocating a temporary std::string

#include <iostream>
#include <unordered_map>
#include <string>
#include <string_view>
#include <chrono>
#include <functional>

struct StringHash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }

    size_t operator()(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }

    size_t operator()(const char* s) const {
        return std::hash<std::string_view>{}(s);
    }
};

struct StringEqual {
    using is_transparent = void;

    bool operator()(std::string_view a, std::string_view b) const {
        return a == b;
    }
};

constexpr int TPS = 150;

struct MerchantInfo {
    std::string name;
    std::string mcc;
    std::string city;
};

template<typename Func>
double measure_ms(Func&& f, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) f();
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

int main() {
    std::cout << "=== Heterogeneous Lookup: Zero-Alloc Merchant Resolution ===\n\n";

    std::unordered_map<std::string, MerchantInfo> traditional;
    std::unordered_map<std::string, MerchantInfo, StringHash, StringEqual> transparent;

    constexpr int TOTAL_MIDS = 500'000;
    for (int i = 0; i < TOTAL_MIDS; ++i) {
        std::string mid = "MID_" + std::to_string(100000 + i);
        MerchantInfo info{"MERCHANT_" + std::to_string(i), "5411", "NEW YORK"};
        traditional[mid] = info;
        transparent[mid] = info;
    }

    // In real acquirer code, the merchant_id arrives as a field
    // parsed from an ISO 8583 message — often as a string_view
    // into the raw message buffer.
    std::string_view mid_from_message = "MID_105000";
    const char* mid_from_c_api = "MID_105000";

    std::cout << "Incoming ISO 8583 field (string_view): " << mid_from_message << "\n\n";

    std::cout << "Traditional (allocates temporary std::string):\n";
    std::cout << "  auto it = cache.find(std::string(sv));\n";
    auto it1 = traditional.find(std::string(mid_from_message));
    std::cout << "  Found: " << (it1 != traditional.end()) << "\n\n";

    std::cout << "Transparent (zero allocation):\n";
    std::cout << "  auto it = cache.find(sv);  // Direct!\n";
    auto it2 = transparent.find(mid_from_message);
    std::cout << "  Found: " << (it2 != transparent.end()) << "\n\n";

    constexpr int TRANSACTIONS = TPS * 60 * 60;

    double traditional_time = measure_ms([&]() {
        volatile auto it = traditional.find(std::string(mid_from_message));
    }, TRANSACTIONS);

    double transparent_time = measure_ms([&]() {
        volatile auto it = transparent.find(mid_from_message);
    }, TRANSACTIONS);

    std::cout << "Benchmark (" << TRANSACTIONS << " merchant lookups):\n";
    std::cout << "  Traditional: " << traditional_time << " ms\n";
    std::cout << "  Transparent: " << transparent_time << " ms\n";
    std::cout << "  Speedup: " << (traditional_time / transparent_time) << "x\n";
    std::cout << "\nAt " << TPS << " TPS over an hour, avoiding string allocations on every auth matters.\n";

    return 0;
}
