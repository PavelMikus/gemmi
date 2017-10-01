// Copyright 2017 Global Phasing Ltd.

// Microbenchmark of gemmi::mol::find_element().
// Requires the google/benchmark library. It can be built manually:
// c++ -Wall -O2 -I../include -I$GB/include bench_elem.cpp $GB/src/libbenchmark.a -pthread

#include "gemmi/elem.hpp"
#include <benchmark/benchmark.h>

inline void run(benchmark::State& state, const char (&el)[10][3]) {
  while (state.KeepRunning())
    for (int i = 0; i != 10; ++i)
      benchmark::DoNotOptimize(gemmi::mol::find_element(el[i]));
}

static void pyridoxine_elements(benchmark::State& state) {
  constexpr char el[10][3] =
      { "C", "C", "N", "C", "C", "C", "C", "O", "C", "O", /*"C", "O"*/ };
  run(state, el);
}

static void common_elements(benchmark::State& state) {
  constexpr char el[10][3] =
      { "C", "N", "O", "S", "Mg", "C", "C", "O", "H", "N" };
  run(state, el);
}

static void various_elements(benchmark::State& state) {
  constexpr char el[10][3] =
      { "Se", "C", "Mg", "P", "Br", "Cl", "C", "O", "Ca", "Si" };
  run(state, el);
}

static void heavy_elements(benchmark::State& state) {
  constexpr char el[10][3] =
      { "Se", "Pb", "Au", "Hg", "Br", "Po", "W", "Ag", "U", "Og" };
  run(state, el);
}

BENCHMARK(pyridoxine_elements);
BENCHMARK(common_elements);
BENCHMARK(various_elements);
BENCHMARK(heavy_elements);
BENCHMARK_MAIN()

/* Output from my desktop:

-----------------------------------------------------------
Benchmark                    Time           CPU Iterations
-----------------------------------------------------------
pyridoxine_elements         11 ns         11 ns   64656977
common_elements             16 ns         16 ns   43586668
various_elements            86 ns         86 ns    8055870
heavy_elements             332 ns        332 ns    2142365
*/

// vim:sw=2:ts=2:et:path^=../include,../third_party
