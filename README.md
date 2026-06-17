# **O(1) or O(no-no-no): Mastering the unordered_map**

Companion code and slides for the 2026 talk on `std::unordered_map`, hashing, and container choice in modern C++.

TAGS: std::unordered_map, performance, stl, containers, hashing, modern c++, security, cache

---

## Versions & status

- **Current: ~90-minute "ACCU on Sea" edition.** The slide deck (`slides/index.html`, reveal.js) is the expanded version. It adds a correctness pitfall, a HashDoS/security section, open-addressing alternatives, a memory deep dive, a profiling/production section, and a real-codebase case study on top of the original.
- **Original: 50-minute "CppOnline" edition.** Preserved in this repo's git history.
- **Title card:** `slides/accu-on-sea-title-card.png` (swap the one `<img>` on slide 1 to switch conferences).

Build the code examples with CMake:

```sh
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Binaries land in `bin/`. Requires a C++23 compiler (GCC 15 recommended); third-party deps (`<flat_map>`, Boost) are `__has_include`-guarded so a fresh clone always builds.

View the slides by opening `slides/index.html` in a browser.

---

## Abstract

In modern C++, `std::unordered_map` has become the de facto #2 container, driven by the performance-critical need for average-case O(1) key-value lookups. This talk moves beyond "just use a hash map" to the real-world implications of that choice.

We benchmark the classic `std::map` (red-black tree) against `std::unordered_map` (hash table) to see exactly what you gain — and what you give up. Then we open the hood: hash functions, buckets, separate chaining, load factor, and rehashing. From there we walk the ways an "average-case O(1)" collapses:

- **Correctness first.** `unordered` means unordered — swap a map you iterate for output (a wire format, a signature, a golden test) and you change behavior, silently.
- **The hash.** What makes a *good* hash function, and how a bad one funnels everything into one bucket.
- **The collision.** Profiling the O(n) worst case — including when an attacker causes it on purpose (HashDoS).
- **The rehash.** Load factor, the hidden stop-the-world cost, and how `reserve()` prevents it.

We finish with modern alternatives (C++23 `flat_map`, open-addressing hash maps, heterogeneous lookup), a memory and cache reality check, production concerns (profiling, concurrency), and a decision framework — validated against a real payment-authorization service. You'll leave knowing not just *which* container, but *how to be sure*.

### Abstract (concise)

`std::vector` is our default sequence — what's our default *associative* container? This talk argues it's `std::unordered_map`, benchmarks why, then pivots to the dangers: an "average-case O(1)" that degrades to O(n), a correctness trap hiding in iteration order, and hash-collision denial-of-service. You'll leave with a practical framework for choosing between `unordered_map`, `map`, `flat_map`, and open-addressing maps — and a healthy instinct to *measure first*.

---

## Talk outline (90-minute edition)

1. **Introduction — the new default.** Why `unordered_map` is the #2 container.
2. **The showdown: `map` vs `unordered_map`.** Live benchmark; O(log n) vs O(1) at scale; the honest caveat that O(1) is in *N*, not in key length *k*.
3. **Under the hood.** Hash → bucket → chain; load factor; why the standard effectively *mandates* node-based separate chaining.
4. **Pitfall 0: it's in the name (correctness).** Iteration order is part of your contract. A distilled real example: an ordered map iterated to build a wire message that is then hashed — swap the container and the hash breaks. (`src/22`)
5. **The three performance pitfalls.** Bad hash (`src/04`, `src/05`), collision catastrophe (`src/06`), hidden rehash and `reserve()` (`src/07`, `src/08`).
6. **Security: HashDoS & CVEs.** Crafted-collision denial-of-service, the 2011 industry-wide event, and mitigations: randomized/keyed hashing. (`src/17`, `src/20`)
7. **Beyond `unordered_map`.** C++23 `flat_map` (`src/09`), C++20 heterogeneous lookup with the SSO caveat (`src/10`), iterator/reference stability (`src/11`).
8. **Open addressing & faster maps.** Why chaining is cache-hostile; `boost::unordered_flat_map`, abseil SwissTable, `ankerl::unordered_dense`; the reference-stability trade-off. (`src/13`)
9. **Memory deep dive.** Per-element overhead (measured), the cache cliff, allocators, and stdlib differences. (`src/14`, `src/15`)
10. **Profiling & production.** Cache-miss counters, runtime bucket inspection, and concurrency: sharding vs a single lock — and the humbling result that the container is often *not* the bottleneck. (`src/16`, `src/18`, `src/19`, `src/21`)
11. **Case study: auditing a real auth service.** Every map walked through the framework — switch, keep (rehash churn), never (ordering). The biggest win turned out to be hoisting a lock, not changing a container.
12. **The decision framework.** A flowchart for `vector` / `map` / `unordered_map` / `flat_map` / concurrent maps, plus the pro-tips checklist.

---

## Source examples (`src/`)

All build under `-std=c++23 -O2`; payments-themed for a consistent narrative. Third-party deps (`<flat_map>`, Boost) are `__has_include`-guarded so a fresh clone always builds.

| # | File | Demonstrates |
|---|------|--------------|
| 01 | `benchmark_map_vs_unordered_txn` | Lookup latency vs container size |
| 02 | `benchmark_simple_txn` | Minimal slide-friendly benchmark |
| 03 | `under_the_hood_txn` | Buckets, load factor, distribution |
| 04 | `pitfall1_bad_hash_txn` | Single-field hash → one giant bucket |
| 05 | `pitfall1_hash_combine_txn` | Boost-style `hash_combine` done right |
| 06 | `pitfall2_collision_catastrophe_txn` | Forced collisions → O(n) meltdown |
| 07 | `pitfall3_rehash_txn` | Rehash latency spikes as the table grows |
| 08 | `pitfall3_reserve_fix_txn` | `reserve()` removes the spikes |
| 09 | `cpp23_flat_map_txn` | C++23 `std::flat_map` for read-mostly tables |
| 10 | `cpp20_heterogeneous_lookup_txn` | Transparent hashing; allocation-free lookup |
| 11 | `iterator_stability_txn` | Rehash invalidates pointers; `map` doesn't |
| 12 | `decision_framework_txn` | Scenario → container recommendation |
| 13 | `open_addressing_compare` | Chaining vs flat hash map (Boost/fallback) |
| 14 | `memory_overhead` | Per-element bytes via a counting allocator |
| 15 | `cache_effects` | The cache cliff: O(1) constant climbing |
| 16 | `bucket_inspection` | Detect a hot bucket at runtime |
| 17 | `hashdos_attack` | Crafted collisions → O(n) (HashDoS) |
| 18 | `sharded_concurrent` | Sharding vs a single global mutex |
| 19 | `perf_workload` | Clean binary to run under `perf`/Instruments |
| 20 | `randomized_hashing` | Per-process salt defeats precomputed attacks |
| 21 | `locking_dominates` | The container is rarely your bottleneck |
| 22 | `ordering_is_load_bearing` | Iteration order is part of the contract |

---

## Design note: why `std::unordered_map` must use separate chaining

A recurring thread in the talk (Under the Hood, Open addressing, and the decision tree) is *why* `std::unordered_map` is node-based — and therefore cache-hostile. Two standard requirements force it:

1. **The bucket interface.** The standard requires a bucket-oriented API — `bucket_count()`, `bucket(key)`, `bucket_size(n)`, and *local iterators* `begin(n)`/`end(n)` that walk a single bucket. That only makes sense if each bucket is a discrete, enumerable chain of elements.

2. **Reference/pointer stability.** Inserting elements and rehashing must **not** invalidate pointers or references to existing elements — only *iterators* are invalidated by a rehash (erase invalidates only references to the erased element). So an address obtained via `&map[key]` must stay valid across a grow + rehash.

**Why that forces node-based chaining.** Stability means element objects can never move in memory. A rehash may re-bucket and re-link, but must not relocate the key/value pairs. The only way to honor that is to allocate each element in its own **heap node** and have buckets hold *pointers* into those nodes; a rehash then just rewires bucket pointers while the nodes stay put. Combined with the bucket-enumeration API, that *is* separate chaining. The standard never writes "use separate chaining," but the combination leaves no other conforming implementation in practice — which is why libstdc++, libc++, and MSVC all do exactly this. It's also the source of the cache misses measured in `src/15_cache_effects.cpp`: lookups pointer-chase to nodes scattered across the heap.

**Why the faster maps exist.** Flat hash maps (`absl::flat_hash_map`, `boost::unordered_flat_map`, `ankerl::unordered_dense`) store elements inline in one contiguous array. Growing that array reallocates and **moves** every element, invalidating references — i.e. they get their speed by *breaking the guarantee the standard mandates*. That's why `std::unordered_map` can't be swapped for a SwissTable internally, and why the fast maps had to ship as separate libraries rather than as the standard container.

> Nuance for Q&A: it isn't *literally* spelled out as "separate chaining," but the bucket interface plus reference stability across rehash make node-based chaining the only conforming implementation — which is why all three major standard libraries use it.

---

## Further reading / cross-references

**Standard containers**
- [`std::unordered_map`](https://en.cppreference.com/w/cpp/container/unordered_map) · [`std::map`](https://en.cppreference.com/w/cpp/container/map) · [`std::flat_map` (C++23)](https://en.cppreference.com/w/cpp/container/flat_map) — cppreference
- Heterogeneous lookup: [P0919 — Heterogeneous lookup for unordered containers](https://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p0919r1.html) · [walkthrough (C++ Stories)](https://www.cppstories.com/2021/heterogeneous-access-cpp20/)

**Hashing & hash functions**
- [Boost.ContainerHash — `hash_combine`](https://www.boost.org/doc/libs/latest/libs/container_hash/doc/html/hash.html)
- Background: [Red–black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree) · [Open addressing](https://en.wikipedia.org/wiki/Open_addressing)

**Faster / open-addressing maps**
- [`boost::unordered_flat_map`](https://www.boost.org/doc/libs/latest/libs/unordered/doc/html/unordered/reference/unordered_flat_map.html) · [Inside `boost::unordered_flat_map`](http://bannalia.blogspot.com/2022/11/inside-boostunorderedflatmap.html)
- Abseil SwissTable: [design notes](https://abseil.io/about/design/swisstables) · [blog](https://abseil.io/blog/20180927-swisstables) · [CppCon 2017 — Matt Kulukundis, "Designing a Fast, Efficient, Cache-friendly Hash Table, Step by Step"](https://www.youtube.com/watch?v=ncHmEUmJZf4)
- [`ankerl::unordered_dense`](https://github.com/martinus/unordered_dense) · [hash-map benchmarks](https://martin.ankerl.com/2022/08/27/hashmap-bench-01/)

**Security — HashDoS**
- [oCERT-2011-003 — multiple implementations DoS via hash collisions](https://ocert.org/advisories/ocert-2011-003.html) · [oss-security thread (CVE list)](https://www.openwall.com/lists/oss-security/2011/12/30/4)
- Mitigation: [PEP 456 — Secure and interchangeable hash algorithm (SipHash)](https://peps.python.org/pep-0456/) · [Python adopts SipHash (LWN)](https://lwn.net/Articles/574761/)

**Concurrency**
- [folly `ConcurrentHashMap`](https://github.com/facebook/folly) · [oneTBB `concurrent_hash_map`](https://github.com/oneapi-src/oneTBB)

---

> The case-study material is distilled from a real payment-authorization service used only as reference; that source is not part of this repo.
