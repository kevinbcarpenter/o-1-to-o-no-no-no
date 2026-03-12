cmake_minimum_required(VERSION 3.20)
project(unordered_map_talk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output binaries to bin folder (separated by build type)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin/release")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin/debug")
endif()

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
