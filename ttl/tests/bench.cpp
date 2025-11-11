#include <benchmark/benchmark.h>
#include <bits/algo.hpp>
#include <bits/ttl/counter.hpp>
#include <bits/ttl/file_sink.hpp>
#include <bits/ttl/runtime.hpp>
#include <cmath>
#include <memory>
#include <span>
#include "ttl.hpp"

using namespace bits::ttl;

template <typename HistogramType>
static double HistogramAdapter(const std::vector<double>& v) {
  HistogramType hist;
  std::span<double> span_v(const_cast<double*>(v.data()), v.size());
  return hist(span_v);
}

static void BM_MetricRecord(benchmark::State& state) {
  if (state.thread_index() == 0) {
    Ttl::init("discard://");
  }

  Counter c("bench.example");

  double value = 100.0;
  for (auto _ : state) {
    c += value;
    value += 0.1;
    benchmark::DoNotOptimize(value);
  }

  if (state.thread_index() == 0) {
    Ttl::shutdown();
  }
}

BENCHMARK(BM_MetricRecord)
    ->Threads(1)
    ->ComputeStatistics("max", HistogramAdapter<bits::Histogram<100, 100>>)
    ->ComputeStatistics("p99", HistogramAdapter<bits::Histogram<99, 100>>);

BENCHMARK(BM_MetricRecord)
    ->Threads(2)
    ->ComputeStatistics("max", HistogramAdapter<bits::Histogram<100, 100>>)
    ->ComputeStatistics("p99", HistogramAdapter<bits::Histogram<99, 100>>);

BENCHMARK(BM_MetricRecord)
    ->Threads(4)
    ->ComputeStatistics("max", HistogramAdapter<bits::Histogram<100, 100>>)
    ->ComputeStatistics("p99", HistogramAdapter<bits::Histogram<99, 100>>);

BENCHMARK(BM_MetricRecord)
    ->Threads(8)
    ->ComputeStatistics("max", HistogramAdapter<bits::Histogram<100, 100>>)
    ->ComputeStatistics("p99", HistogramAdapter<bits::Histogram<99, 100>>);

BENCHMARK(BM_MetricRecord)
    ->Threads(60)
    ->ComputeStatistics("max", HistogramAdapter<bits::Histogram<100, 100>>)
    ->ComputeStatistics("p99", HistogramAdapter<bits::Histogram<99, 100>>);

BENCHMARK_MAIN();
