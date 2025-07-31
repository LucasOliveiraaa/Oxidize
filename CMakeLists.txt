cmake_minimum_required(VERSION 3.14)
project(oxidize_tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch Catch2 (already in your config)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.5.4
)
FetchContent_MakeAvailable(Catch2)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Fetch Google Benchmark
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.7.0 # or latest stable release
)
FetchContent_MakeAvailable(benchmark)

# Add your test executable
add_executable(oxidize_tests tests/oxidize_tests.cpp)
target_include_directories(oxidize_tests PRIVATE include)

# Link Catch2 and Boost.Stacktrace
target_link_libraries(oxidize_tests PRIVATE Catch2::Catch2WithMain ${Boost_LIBRARIES} backtrace)
target_include_directories(oxidize_tests PRIVATE ${Boost_INCLUDE_DIRS})

target_compile_definitions(oxidize_tests PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)

# Link Google Benchmark
target_link_libraries(oxidize_tests PRIVATE benchmark::benchmark)

# Enable CTest and register Catch2 tests
include(CTest)
include(Catch)
catch_discover_tests(oxidize_tests)

add_executable(oxidize_benchmarks benchmarks/oxidize_bench.cpp)
target_include_directories(oxidize_benchmarks PRIVATE include)

target_link_libraries(oxidize_benchmarks PRIVATE benchmark::benchmark)
