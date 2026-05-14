#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>
#include <x86intrin.h>

inline std::mt19937& rng() {
    static std::mt19937 gen(42);
    return gen;
}

template <typename T>
inline void do_not_optimize(T const& val) {
    asm volatile("" : : "r,m"(val) : "memory");
}

inline void clobber() {
    asm volatile("" : : : "memory");
}

inline uint64_t cycle_start() {
    _mm_lfence();
    return __rdtsc();
}

inline uint64_t cycle_end() {
    uint32_t aux;
    uint64_t tsc = __rdtscp(&aux);
    _mm_lfence();
    return tsc;
}

struct BenchmarkEntry {
    std::string name;
    int ops_per_run;
    std::function<uint64_t(int)> fn;
};

inline std::vector<BenchmarkEntry>& benchmark_registry() {
    static std::vector<BenchmarkEntry> reg;
    return reg;
}

struct RegisterBenchmark {
    RegisterBenchmark(const char* name, int ops_per_run, std::function<uint64_t(int)> fn) {
        benchmark_registry().push_back({name, ops_per_run, std::move(fn)});
    }
};

inline void run_benchmarks() {
    constexpr int warmup     = 3;
    constexpr int iterations = 10;

    for (auto& bm : benchmark_registry()) {
        bm.fn(warmup);
        uint64_t cycles = bm.fn(iterations);
        double cycles_per_op = static_cast<double>(cycles)
                             / (static_cast<double>(iterations) * bm.ops_per_run);
        std::printf("%-35s  %.2f cycles/op\n", bm.name.c_str(), cycles_per_op);
    }
}

