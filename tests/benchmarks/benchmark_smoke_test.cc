#include <benchmark/benchmark.h>

namespace {

void BM_BazelBenchmarkSkeleton(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(42);
  }
}
BENCHMARK(BM_BazelBenchmarkSkeleton);

}  // namespace
