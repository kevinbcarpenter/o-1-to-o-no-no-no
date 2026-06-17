// Section 5: HashDoS mitigation - Acquirer Edition
// A per-process random salt makes your hash unpredictable across runs, so an
// attacker can't precompute colliding inputs. This is what Python, Rust, and
// modern runtimes do (SipHash + random key). Demo: same keys, different
// bucket distribution each run; precomputed collisions stop working.

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <iomanip>

// Process-lifetime salt, seeded once from the OS RNG.
static const uint64_t kSalt = [] {
    std::random_device rd;
    return (uint64_t(rd()) << 32) ^ rd();
}();

// Salted hash: mixes the secret salt into a standard hash. An attacker who
// doesn't know kSalt cannot craft guaranteed collisions ahead of time.
struct SaltedHash {
    size_t operator()(const std::string& s) const {
        size_t h = std::hash<std::string>{}(s);
        h ^= kSalt + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

template <typename Map>
size_t worst_bucket(const Map& m) {
    size_t w = 0;
    for (size_t b = 0; b < m.bucket_count(); ++b) w = std::max(w, m.bucket_size(b));
    return w;
}

int main() {
    std::cout << "=== Randomized hashing defeats precomputed collisions ===\n\n";
    std::cout << "Process salt this run: 0x" << std::hex << kSalt << std::dec << "\n";
    std::cout << "(re-run: the salt changes, so any precomputed attack set is stale)\n\n";

    const int N = 20000;
    std::vector<std::string> keys;
    keys.reserve(N);
    for (int i = 0; i < N; ++i) keys.push_back("AUTH" + std::to_string(100000 + i));

    std::unordered_map<std::string, uint32_t> plain;            // default std::hash
    std::unordered_map<std::string, uint32_t, SaltedHash> salted;
    for (int i = 0; i < N; ++i) { plain[keys[i]] = i; salted[keys[i]] = i; }

    std::cout << "Legitimate traffic, both hashes behave well:\n";
    std::cout << "  default std::hash  worst bucket = " << worst_bucket(plain) << "\n";
    std::cout << "  salted hash        worst bucket = " << worst_bucket(salted) << "\n\n";

    std::cout << "The salt costs almost nothing on the happy path, but removes the\n";
    std::cout << "attacker's ability to pick inputs that all land in one bucket.\n";
    std::cout << "In production, prefer a vetted keyed hash (SipHash) over a hand-rolled mix.\n";
    return 0;
}
