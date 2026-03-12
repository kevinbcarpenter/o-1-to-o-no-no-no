// Section 4, Pitfall 1: Bad Hash - Acquirer Edition
// Hashing a Transaction struct by currency alone is a disaster

#include <iostream>
#include <unordered_map>
#include <string>
#include <chrono>

struct Transaction {
    std::string merchant_id;
    std::string card_bin;
    uint32_t amount_cents;
    std::string currency;

    bool operator==(const Transaction& other) const {
        return merchant_id == other.merchant_id
            && card_bin == other.card_bin
            && amount_cents == other.amount_cents
            && currency == other.currency;
    }
};

// BAD HASH - Almost every transaction is "USD", so they all collide!
struct BadHash {
    size_t operator()(const Transaction& t) const {
        return std::hash<std::string>{}(t.currency);
    }
};

// GOOD HASH - Combines all fields
struct GoodHash {
    size_t operator()(const Transaction& t) const {
        size_t h1 = std::hash<std::string>{}(t.merchant_id);
        size_t h2 = std::hash<std::string>{}(t.card_bin);
        size_t h3 = std::hash<uint32_t>{}(t.amount_cents);
        size_t h4 = std::hash<std::string>{}(t.currency);
        size_t seed = h1;
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template<typename Hash>
void benchmark(const std::string& label, int n) {
    std::unordered_map<Transaction, std::string, Hash> txn_cache;

    for (int i = 0; i < n; ++i) {
        Transaction txn{
            "MID_" + std::to_string(i % 500),
            "411111",
            static_cast<uint32_t>(1000 + i),
            "USD"  // 99% of transactions are USD
        };
        txn_cache[txn] = "AUTH" + std::to_string(i);
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n; ++i) {
        Transaction txn{
            "MID_" + std::to_string(i % 500),
            "411111",
            static_cast<uint32_t>(1000 + i),
            "USD"
        };
        volatile auto it = txn_cache.find(txn);
    }
    auto elapsed = std::chrono::high_resolution_clock::now() - start;

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    std::cout << label << " (n=" << n << "): " << ms << " ms\n";

    size_t max_bucket = 0;
    for (size_t i = 0; i < txn_cache.bucket_count(); ++i) {
        max_bucket = std::max(max_bucket, txn_cache.bucket_size(i));
    }
    std::cout << "  Max bucket size: " << max_bucket << " (ideal: ~1)\n\n";
}

int main() {
    std::cout << "=== Bad Hash: Hashing Transactions by Currency Alone ===\n\n";
    std::cout << "When 99% of your transactions are USD,\n";
    std::cout << "hashing on currency puts them ALL in one bucket.\n\n";

    for (int n : {1000, 5000, 10000}) {
        benchmark<BadHash>("BadHash ", n);
        benchmark<GoodHash>("GoodHash", n);
        std::cout << std::string(50, '-') << "\n";
    }

    return 0;
}
