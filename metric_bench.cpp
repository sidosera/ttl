#include <benchmark/benchmark.h>
#include <algorithm>
#include <cmath>

#include "factory.hpp"

using namespace bits::ttl;

struct NoopSink : public Sink {
  void publish(const Event&) override {}
};

template <int PctNum, int PctDen = 100>
static double PercentileFn(const std::vector<double>& v) {
  if (v.empty())
    return 0.0;

  std::vector<double> copy = v;
  std::sort(copy.begin(), copy.end());

  const double q = std::clamp(double(PctNum) / double(PctDen), 0.0, 1.0);
  const double pos = q * (copy.size() - 1);
  const size_t idx = static_cast<size_t>(std::floor(pos));
  const double frac = pos - idx;

  if (idx + 1 < copy.size())
    return copy[idx] + frac * (copy[idx + 1] - copy[idx]);
  return copy[idx];
}

static void BM_MetricRecord(benchmark::State& state) {
  NoopSink sink;

  Tag tags[] = {{"host", "localhost"}};
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 1};
  config.tags[0] = tags[0];

  auto factory = TtlFactory{config};
  auto& m = factory.measure("bench.latency");

  double value = 100.0;
  for (auto _ : state) {
    m += value;
    value += 0.1;
    benchmark::DoNotOptimize(value);
  }
}

BENCHMARK(BM_MetricRecord)
    ->Threads(1)
    ->ComputeStatistics("max", PercentileFn<100>)
    ->ComputeStatistics("p99", PercentileFn<99>);

BENCHMARK(BM_MetricRecord)
    ->Threads(2)
    ->ComputeStatistics("max", PercentileFn<100>)
    ->ComputeStatistics("p99", PercentileFn<99>);

BENCHMARK(BM_MetricRecord)
    ->Threads(4)
    ->ComputeStatistics("max", PercentileFn<100>)
    ->ComputeStatistics("p99", PercentileFn<99>);

BENCHMARK(BM_MetricRecord)
    ->Threads(8)
    ->ComputeStatistics("max", PercentileFn<100>)
    ->ComputeStatistics("p99", PercentileFn<99>);

BENCHMARK(BM_MetricRecord)
    ->Threads(60)
    ->ComputeStatistics("max", PercentileFn<100>)
    ->ComputeStatistics("p99", PercentileFn<99>);

BENCHMARK_MAIN();
