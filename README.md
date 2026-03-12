# **Title: O(1) or O(no-no-no): Mastering the unordered_map**

### mastering_the_unordered_map
2026 Talk about unordered_map and hashing algorithms in C++

TAGS: std::unordered_map, performance, stl, containers, hashing, modern c++

---

### **Abstract**

In modern C++, std::unordered_map is the de facto #2 container. Its dominance signifies a fundamental shift in application design: the performance-critical need for O(1) average-case key-value lookups.

Join as we move beyond "just use a hash map" and explore the critical, real-world implications of this choice. We'll start by benchmarking the classic `std::map` (red-black tree) against `std::unordered_map` (hash table) to understand exactly what you gain-and what you might lose.

However this power comes with risks. An average-case O(1) can quickly degrade to a catastrophic O(n) without warning. We will profile and dissect the actual costs of using a hash map:

* **The Hash:** What makes a *good* hash function? We'll go beyond `std::hash` and see how to write effective, fast hashers.
* **The Collision:** How do different collision-handling strategies impact performance and memory?
* **The Re-hash:** What is "load factor," and when does the hidden cost of a full table re-hash destroy your performance gains?

We'll conclude with a practical decision-making framework for when to choose `std::unordered_map`, when to fall back on the ordered `std::map`, and how `std::string`-the container we often forget is a container-fits into this modern landscape.

You will leave this session knowing precisely which data structure to deploy for maximum performance.

---

### **Talk Outline**

1. **`map` vs. `unordered_map`**
    * We establish why `std::unordered_map` is the de facto #2 container in modern C++.
    * A compelling live benchmark demonstrates the staggering performance difference between `std::map` (O(log n)) and `std::unordered_map` (average O(1)) for lookup-heavy workloads.

2. **The hash in more detail**
    * A brief, practical explanation of how a hash table actually works (hash functions, buckets, and separate chaining). This provides the necessary context for understanding the pitfalls.

3. **Three Pitfalls (When O(1) Becomes O(n))**
    * Dissect the three ways `std::unordered_map` performance collapses:
        * The Bad Hash: How to (and how not to) write hash functions for custom types.
        * The Collision Catastrophe: Profiling the worst-case O(n) scenario.
        * The Hidden Re-hash: The latency spike and how to prevent it with `reserve()`.

4. **Choosing Wisely**
    * We present a simple, actionable decision tree for choosing between `std::vector`, `std::map`, and `std::unordered_map`, empowering attendees to make the right choice under pressure.

---