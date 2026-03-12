// Section 4, Pitfall 1: Proper hash_combine - Acquirer Edition
// Boost-style hash_combine for Transaction structs

#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstdint>

template <typename T>
inline void hash_combine(size_t& seed, const T& val) {
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T, typename... Rest>
inline void hash_combine(size_t& seed, const T& val, const Rest&... rest) {
    hash_combine(seed, val);
    (hash_combine(seed, rest), ...);
}

template <typename... Args>
inline size_t hash_values(const Args&... args) {
    size_t seed = 0;
    (hash_combine(seed, args), ...);
    return seed;
}

struct Transaction {
    std::string merchant_id;
    std::string card_bin;
    uint32_t amount_cents;
    std::string currency;
    uint64_t timestamp_ms;

    bool operator==(const Transaction& other) const = default;
};

struct TransactionHash {
    size_t operator()(const Transaction& t) const {
        return hash_values(t.merchant_id, t.card_bin,
                           t.amount_cents, t.currency, t.timestamp_ms);
    }
};

int main() {
    std::unordered_map<Transaction, std::string, TransactionHash> auth_cache;

    auth_cache[{"MID_100001", "411111", 5000, "USD", 1719000000000}] = "AUTH_A1B2C3";
    auth_cache[{"MID_200045", "523456", 12599, "USD", 1719000001000}] = "AUTH_D4E5F6";
    auth_cache[{"MID_300078", "370000", 75000, "USD", 1719000002000}] = "AUTH_G7H8I9";

    std::cout << "Hash values for different transactions:\n";
    std::cout << std::string(60, '-') << "\n";

    TransactionHash hasher;
    for (const auto& [txn, auth_code] : auth_cache) {
        std::cout << txn.merchant_id << " | BIN:" << txn.card_bin
                  << " | $" << (txn.amount_cents / 100) << "." << (txn.amount_cents % 100)
                  << " -> hash: " << hasher(txn) << "\n";
    }

    // Show that similar transactions get very different hashes
    std::cout << "\nSimilar transactions, different hashes:\n";
    std::cout << "MID_100001, $50.00, ts:1000 -> "
              << hasher({"MID_100001", "411111", 5000, "USD", 1719000000000}) << "\n";
    std::cout << "MID_100001, $50.01, ts:1000 -> "
              << hasher({"MID_100001", "411111", 5001, "USD", 1719000000000}) << "\n";
    std::cout << "MID_100001, $50.00, ts:1001 -> "
              << hasher({"MID_100001", "411111", 5000, "USD", 1719000001000}) << "\n";

    std::cout << "\nOne-cent or one-millisecond difference produces completely different hashes.\n";

    return 0;
}
