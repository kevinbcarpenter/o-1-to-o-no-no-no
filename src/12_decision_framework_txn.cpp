// Section 6: Decision Framework - Acquirer Edition
// Real acquirer scenarios mapped to the right container

#include <iostream>
#include <string>

void print_recommendation(bool need_ordering, bool need_stability,
                          bool know_size, bool custom_key) {
    std::cout << "\n=== Recommendation ===\n\n";

    if (need_ordering) {
        std::cout << "Use: std::map\n";
        std::cout << "Reason: You need sorted iteration order.\n";
        std::cout << "Complexity: O(log n) lookup, insert, delete\n";
        return;
    }

    if (need_stability) {
        std::cout << "Use: std::map\n";
        std::cout << "Reason: You need iterator/reference stability.\n";
        std::cout << "Complexity: O(log n) lookup, insert, delete\n";
        return;
    }

    std::cout << "Use: std::unordered_map\n";
    std::cout << "Reason: O(1) average-case is your best bet.\n";
    std::cout << "Complexity: O(1) average lookup, insert, delete\n\n";

    std::cout << "Checklist:\n";
    if (custom_key) {
        std::cout << "  [!] Custom key type - implement a good hash function!\n";
        std::cout << "      Use hash_combine for multi-field keys.\n";
    }
    if (know_size) {
        std::cout << "  [!] Known size - call reserve(expected_size)!\n";
        std::cout << "      This prevents costly rehashing.\n";
    }
    if (!custom_key && !know_size) {
        std::cout << "  [OK] Standard key type with default hash.\n";
        std::cout << "  [OK] Dynamic size - rehashing is acceptable.\n";
    }
}

int main() {
    std::cout << "=== Acquirer Container Decision Framework ===\n\n";

    std::cout << "Scenario 1: Auth Code Cache (auth_code -> Transaction)\n";
    std::cout << "  - No ordering needed\n";
    std::cout << "  - No pointer stability needed\n";
    std::cout << "  - Size varies (auths come and go)\n";
    std::cout << "  - Standard key type (string)\n";
    print_recommendation(false, false, false, false);

    std::cout << "\n" << std::string(60, '=') << "\n";

    std::cout << "\nScenario 2: Settlement Report (timestamp -> TxnBatch)\n";
    std::cout << "  - Need sorted iteration (chronological order for reconciliation)\n";
    print_recommendation(true, false, false, false);

    std::cout << "\n" << std::string(60, '=') << "\n";

    std::cout << "\nScenario 3: In-Flight Auth Registry (TXN_ID -> AuthRecord*)\n";
    std::cout << "  - Reversal/void handlers hold pointers to auth records\n";
    std::cout << "  - Pointers MUST remain valid across new insertions\n";
    print_recommendation(false, true, false, false);

    std::cout << "\n" << std::string(60, '=') << "\n";

    std::cout << "\nScenario 4: BIN Lookup Table (BIN -> IssuerInfo)\n";
    std::cout << "  - Loaded at startup from config\n";
    std::cout << "  - Known size (~300,000 BIN ranges)\n";
    std::cout << "  - Queried on every auth request\n";
    std::cout << "  - Standard key type (string)\n";
    print_recommendation(false, false, true, false);

    std::cout << "\n" << std::string(60, '=') << "\n";

    std::cout << "\nScenario 5: Merchant Risk Cache (MerchantProfile -> RiskScore)\n";
    std::cout << "  - No ordering needed\n";
    std::cout << "  - Known size (~50,000 active merchants)\n";
    std::cout << "  - Custom key type (MerchantProfile struct)\n";
    print_recommendation(false, false, true, true);

    return 0;
}
