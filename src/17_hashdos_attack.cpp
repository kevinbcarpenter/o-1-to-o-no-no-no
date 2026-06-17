// Section 5: HashDoS - Acquirer Edition  [LIVE DEMO 2]
// An attacker who knows your hash can craft inputs that all collide, turning
// an O(1) lookup into O(n) and your service into a tarpit. This is HashDoS.
//
// We use a deliberately weak hash so the colliding inputs are easy to generate
// live. Real attacks target known weaknesses in a server's actual hash.

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>
#include <iomanip>

// Weak hash an attacker can defeat by hand: only sums byte values.
// "ab" and "ba" collide; so do any anagrams. Easy to mass-produce collisions.
struct WeakSumHash {
    size_t operator()(const std::string& s) const {
        size_t h = 0; for (char c : s) h += static_cast<unsigned char>(c);
        return h;
    }
};

// Generate n distinct strings that ALL hash to the same WeakSumHash value.
// Trick: fixed byte-sum by padding with 'A's and encoding an index in a tail
// that preserves the sum (swap 'A'(65)+'A'(65) for 'B'(66)+'@'(64), etc.).
std::vector<std::string> craft_collisions(int n) {
    std::vector<std::string> out;
    // Base string of 'A's; we mutate pairs to keep the sum constant.
    // Simpler robust approach: build distinct strings, then pad each so the
    // byte-sum equals a fixed target.
    const size_t target = 200 * 64;  // generous fixed sum
    for (int i = 0; i < n; ++i) {
        std::string s = "atk" + std::to_string(i);
        size_t sum = 0; for (char c : s) sum += static_cast<unsigned char>(c);
        // Pad with '\1' bytes (value 1) until we can hit target exactly, then
        // append one byte to top it off. Keeps every string distinct via the
        // index, but all share the same byte-sum => same WeakSumHash.
        while (sum < target - 255) { s.push_back('\1'); sum += 1; }
        s.push_back(static_cast<char>(target - sum));  // final byte closes the sum
        out.push_back(std::move(s));
    }
    return out;
}

template <typename Map>
double time_lookups(Map& m, const std::vector<std::string>& keys) {
    volatile size_t sink = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (auto& k : keys) { auto it = m.find(k); if (it != m.end()) sink += it->second; }
    auto t1 = std::chrono::high_resolution_clock::now();
    (void)sink;
    return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

int main() {
    std::cout << "=== HashDoS: crafting a collision flood ===\n\n";

    for (int n : {1000, 2000, 4000, 8000}) {
        auto attack_keys = craft_collisions(n);

        std::unordered_map<std::string, uint32_t> strong;             // default std::hash
        std::unordered_map<std::string, uint32_t, WeakSumHash> weak;  // attacker's target
        for (int i = 0; i < n; ++i) { strong[attack_keys[i]] = i; weak[attack_keys[i]] = i; }

        double strong_ms = time_lookups(strong, attack_keys);
        double weak_ms   = time_lookups(weak,   attack_keys);

        // Confirm the attack worked: how big is the worst bucket under the weak hash?
        size_t worst = 0;
        for (size_t b = 0; b < weak.bucket_count(); ++b) worst = std::max(worst, weak.bucket_size(b));

        std::cout << "n=" << std::setw(5) << n
                  << "  strong hash: " << std::fixed << std::setprecision(2) << std::setw(7) << strong_ms << " ms"
                  << "   weak hash: " << std::setw(8) << weak_ms << " ms"
                  << "   (worst bucket under attack: " << worst << ")\n";
    }

    std::cout << "\nUnder attack the weak map degenerates to one giant bucket: O(n) per lookup.\n";
    std::cout << "Mitigation: a per-process random salt (see src/20). Real CVEs: CVE-2011-3414\n";
    std::cout << "(ASP.NET), oCERT-2011-003 (Rails/Python/Node hash-collision DoS).\n";
    return 0;
}
