cmake_minimum_required(VERSION 3.14)
project(Oxidize CXX)

# === Build Configuration ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# === Dependencies ===
include(FetchContent)

# --- Catch2 ---
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# --- Google Benchmark ---
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.7.0
)
FetchContent_MakeAvailable(benchmark)

# --- fmt ---
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        10.1.1  # or latest stable version
)
FetchContent_MakeAvailable(fmt)

# === Include Project Headers ===
include_directories(${CMAKE_SOURCE_DIR}/include)

# === Tests ===
enable_testing()
add_executable(oxidize_tests
  tests/arc.cpp
  tests/either.cpp
  tests/panic.cpp
  tests/result.cpp
  tests/tuple.cpp
  tests/vec.cpp
  tests/format.cpp
  tests/mutex.cpp
  tests/rwlock.cpp
  # add more test files here
)
target_link_libraries(oxidize_tests
  PRIVATE
    Catch2::Catch2WithMain
    benchmark::benchmark
    fmt::fmt
)
include(Catch)
catch_discover_tests(oxidize_tests)

# === Benchmarks ===
add_executable(oxidize_benchmarks
  benchmarks/oxidize_bench.cpp
  # add more benchmark files here
)
target_link_libraries(oxidize_benchmarks PRIVATE benchmark::benchmark fmt::fmt)
