// Section 5: Iterator Stability - Acquirer Edition
// Holding pointers to in-flight authorization records

#include <iostream>
#include <map>
#include <unordered_map>
#include <string>

struct AuthRecord {
    std::string auth_code;
    uint32_t amount_cents;
    std::string status;
};

int main() {
    std::cout << "=== Iterator Stability: In-Flight Auth Records ===\n\n";
    std::cout << "Acquirer systems often cache pointers to in-flight auths\n";
    std::cout << "for reversal/void processing. Rehash breaks those pointers.\n\n";

    std::cout << "--- std::unordered_map (DANGER!) ---\n\n";
    {
        std::unordered_map<std::string, AuthRecord> auth_cache;
        auth_cache.reserve(4);

        auth_cache["TXN_0001"] = {"AUTH_A1B2", 5000, "APPROVED"};
        auth_cache["TXN_0002"] = {"AUTH_C3D4", 12599, "APPROVED"};

        // Reversal handler holds a pointer to the original auth
        AuthRecord* pending_reversal = &auth_cache["TXN_0001"];
        std::cout << "Reversal handler cached ptr to TXN_0001: "
                  << pending_reversal << " -> " << pending_reversal->auth_code << "\n";
        std::cout << "Bucket count: " << auth_cache.bucket_count() << "\n\n";

        std::cout << "New auths flooding in, triggering rehash...\n";
        for (int i = 3; i < 20; ++i) {
            std::string txn = "TXN_" + std::to_string(i);
            auth_cache[txn] = {"AUTH_" + std::to_string(i), static_cast<uint32_t>(i * 1000), "APPROVED"};
        }

        std::cout << "Bucket count after: " << auth_cache.bucket_count() << "\n";
        std::cout << "New address of TXN_0001: " << &auth_cache["TXN_0001"] << "\n";
        std::cout << "\n** Original pointer is DANGLING! **\n";
        std::cout << "** Reversal would crash or corrupt data! **\n\n";
    }

    std::cout << "--- std::map (SAFE) ---\n\n";
    {
        std::map<std::string, AuthRecord> auth_cache;

        auth_cache["TXN_0001"] = {"AUTH_A1B2", 5000, "APPROVED"};
        auth_cache["TXN_0002"] = {"AUTH_C3D4", 12599, "APPROVED"};

        AuthRecord* pending_reversal = &auth_cache["TXN_0001"];
        std::cout << "Reversal handler cached ptr to TXN_0001: "
                  << pending_reversal << " -> " << pending_reversal->auth_code << "\n\n";

        std::cout << "New auths flooding in...\n";
        for (int i = 3; i < 20; ++i) {
            std::string txn = "TXN_" + std::to_string(i);
            auth_cache[txn] = {"AUTH_" + std::to_string(i), static_cast<uint32_t>(i * 1000), "APPROVED"};
        }

        std::cout << "Address of TXN_0001 after: " << &auth_cache["TXN_0001"] << "\n";
        std::cout << "Original pointer still valid: " << pending_reversal->auth_code << "\n";
        std::cout << "\n** std::map guarantees reference stability! **\n";
    }

    std::cout << "\n=== When to use std::map in an acquirer ===\n";
    std::cout << "- In-flight auth records with pending reversals/voids\n";
    std::cout << "- Long-lived references across async processing stages\n";
    std::cout << "- Settlement batches needing sorted iteration by timestamp\n";
    std::cout << "- Guaranteed O(log n) under adversarial input (DoS resistance)\n";

    return 0;
}
