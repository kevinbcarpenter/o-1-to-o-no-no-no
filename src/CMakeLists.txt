cmake_minimum_required(VERSION 3.20)
project(unordered_map_talk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Put all built executables in <project-root>/bin (this CMakeLists lives in src/)
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin")
# Multi-config generators (Xcode, Visual Studio) otherwise add per-config subdirs
foreach(cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} "${PROJECT_ROOT}/bin")
endforeach()

# Optimization flags for benchmarks
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# All example executables
add_executable(01_benchmark_map_vs_unordered_txn 01_benchmark_map_vs_unordered_txn.cpp)
add_executable(02_benchmark_simple_txn 02_benchmark_simple_txn.cpp)
add_executable(03_under_the_hood_txn 03_under_the_hood_txn.cpp)
add_executable(04_pitfall1_bad_hash_txn 04_pitfall1_bad_hash_txn.cpp)
add_executable(05_pitfall1_hash_combine_txn 05_pitfall1_hash_combine_txn.cpp)
add_executable(06_pitfall2_collision_catastrophe_txn 06_pitfall2_collision_catastrophe_txn.cpp)
add_executable(07_pitfall3_rehash_txn 07_pitfall3_rehash_txn.cpp)
add_executable(08_pitfall3_reserve_fix_txn 08_pitfall3_reserve_fix_txn.cpp)
add_executable(09_cpp23_flat_map_txn 09_cpp23_flat_map_txn.cpp)
add_executable(10_cpp20_heterogeneous_lookup_txn 10_cpp20_heterogeneous_lookup_txn.cpp)
add_executable(11_iterator_stability_txn 11_iterator_stability_txn.cpp)
add_executable(12_decision_framework_txn 12_decision_framework_txn.cpp)

# 90-minute expansion: open addressing, memory, profiling, security
add_executable(13_open_addressing_compare 13_open_addressing_compare.cpp)
add_executable(14_memory_overhead 14_memory_overhead.cpp)
add_executable(15_cache_effects 15_cache_effects.cpp)
add_executable(16_bucket_inspection 16_bucket_inspection.cpp)
add_executable(17_hashdos_attack 17_hashdos_attack.cpp)
add_executable(18_sharded_concurrent 18_sharded_concurrent.cpp)
add_executable(19_perf_workload 19_perf_workload.cpp)
add_executable(20_randomized_hashing 20_randomized_hashing.cpp)

# Round 2: real-codebase-informed examples
add_executable(21_locking_dominates 21_locking_dominates.cpp)
add_executable(22_ordering_is_load_bearing 22_ordering_is_load_bearing.cpp)

# Threads for the concurrency examples
find_package(Threads REQUIRED)
target_link_libraries(18_sharded_concurrent PRIVATE Threads::Threads)
target_link_libraries(21_locking_dominates PRIVATE Threads::Threads)

# Optional: real flat hash map for src/13 if Boost is available
find_package(Boost QUIET)
if(Boost_FOUND)
    target_include_directories(13_open_addressing_compare PRIVATE ${Boost_INCLUDE_DIRS})
endif()
