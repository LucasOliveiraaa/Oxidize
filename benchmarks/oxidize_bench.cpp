#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include "Oxidize/vec/Vec.hpp"
#include "Oxidize/string/String.hpp"
#include <new>      // std::nothrow
#include "Oxidize/alloc/Allocator.hpp"
#include "Oxidize/alloc/Layout.hpp"
#include "Oxidize/ptr/NonNull.hpp"

using namespace ox;

// Benchmark std::vector push_back
static void BM_StdVectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> v;
        for (int i = 0; i < state.range(0); ++i) {
            v.push_back(i);
        }
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_StdVectorPushBack)->RangeMultiplier(10)->Range(1000, 100000);

// Benchmark Oxidize Vec push
static void BM_OxVecPush(benchmark::State& state) {
    for (auto _ : state) {
        mut v = Vec<int>::new_();
        for (int i = 0; i < state.range(0); ++i) {
            v.push(move(i));
        }
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_OxVecPush)->RangeMultiplier(10)->Range(1000, 100000);

// Benchmark std::string concatenation
static void BM_StdStringConcat(benchmark::State& state) {
    for (auto _ : state) {
        std::string s;
        for (int i = 0; i < state.range(0); ++i) {
            s += "x";
        }
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_StdStringConcat)->RangeMultiplier(10)->Range(1000, 100000);

// Benchmark Oxidize String concatenation
static void BM_OxStringConcat(benchmark::State& state) {
    for (auto _ : state) {
        mut s = String();
        for (int i = 0; i < state.range(0); ++i) {
            s.push('x');
        }
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_OxStringConcat)->RangeMultiplier(10)->Range(1000, 100000);

static void BM_OxVecU8Push(benchmark::State& state) {
    for (auto _ : state) {
        mut v = Vec<uint8_t>::new_();
        for (int i = 0; i < state.range(0); ++i) {
            v.push('x');
        }
        benchmark::DoNotOptimize(v);
    }
}
BENCHMARK(BM_OxVecU8Push)->RangeMultiplier(10)->Range(1000, 100000);


// Benchmark standard allocation (new/delete)
static void BM_StdAllocIntArray(benchmark::State& state) {
    const size_t count = state.range(0);
    for (auto _ : state) {
        int* ptr = new(std::nothrow) int[count];
        benchmark::DoNotOptimize(ptr);
        delete[] ptr;
    }
}
BENCHMARK(BM_StdAllocIntArray)->RangeMultiplier(10)->Range(1000, 100000);

// Benchmark ox::Allocator allocation/deallocation
static void BM_OxAllocIntArray(benchmark::State& state) {
    const size_t count = state.range(0);
    mut alloc = alloc::Global();
    for (auto _ : state) {
        auto layout = alloc::Layout::for_value<int>().repeat(count);
        auto res = alloc.allocate(layout);

        if (!res.is_ok()) {
            state.SkipWithError("Allocator::allocate failed");
            return;
        }

        auto ptr = res.unwrap();
        benchmark::DoNotOptimize(ptr);

        alloc.deallocate(ptr, layout);
    }
}
BENCHMARK(BM_OxAllocIntArray)->RangeMultiplier(10)->Range(1000, 100000);

static void BM_OxAllocGrow(benchmark::State& state) {
    mut alloc = alloc::Alloc<alloc::Global>();
    for (auto _ : state) {
        auto layout_small = alloc::Layout::for_value<int>().repeat(state.range(0));
        auto layout_large = alloc::Layout::for_value<int>().repeat(state.range(0) * 2);

        auto alloc_result = alloc.allocate(layout_small);
        if (!alloc_result.is_ok()) {
            state.SkipWithError("Allocation failed");
            return;
        }
        auto ptr = alloc_result.unwrap();

        auto grow_result = alloc.grow<int>(ptr.cast<int>(), layout_small, layout_large);
        if (!grow_result.is_ok()) {
            state.SkipWithError("Grow failed");
            return;
        }
        auto grown_ptr = grow_result.unwrap();

        alloc.deallocate(grown_ptr.cast<void>(), layout_large);
    }
}
BENCHMARK(BM_OxAllocGrow)->RangeMultiplier(10)->Range(1000, 100000);

// Benchmark shrink
static void BM_OxAllocShrink(benchmark::State& state) {
    mut alloc = alloc::Alloc<alloc::Global>();
    for (auto _ : state) {
        auto layout_large = alloc::Layout::for_value<int>().repeat(state.range(0) * 2);
        auto layout_small = alloc::Layout::for_value<int>().repeat(state.range(0));

        auto alloc_result = alloc.allocate(layout_large);
        if (!alloc_result.is_ok()) {
            state.SkipWithError("Allocation failed");
            return;
        }
        auto ptr = alloc_result.unwrap();

        auto shrink_result = alloc.shrink<int>(ptr.cast<int>(), layout_large, layout_small);
        if (!shrink_result.is_ok()) {
            state.SkipWithError("Shrink failed");
            return;
        }
        auto shrunk_ptr = shrink_result.unwrap();

        alloc.deallocate(shrunk_ptr.cast<void>(), layout_small);
    }
}
BENCHMARK(BM_OxAllocShrink)->RangeMultiplier(10)->Range(1000, 100000);

BENCHMARK_MAIN();