#include "sampler.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include "sampling_strategies.hpp"

using namespace bits::ttl;

namespace {
int64_t nowUs() {
  return std::chrono::duration_cast<std::chrono::microseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}
}  // namespace

TEST(SamplerTest, BasicYieldAndCapture) {
  Sampler<double> sampler(strategies::uniformSample<double>);

  sampler.yield(1.0);
  sampler.yield(2.0);
  sampler.yield(3.0);

  auto sample = sampler.capture(nowUs());

  ASSERT_TRUE(sample.has_value());
  EXPECT_EQ(sample->count, 3);
  EXPECT_GE(sample->value, 1.0);
  EXPECT_LE(sample->value, 3.0);
}

TEST(SamplerTest, NoCaptureWhenNoData) {
  Sampler<double> sampler(strategies::uniformSample<double>);

  auto sample = sampler.capture(nowUs());

  EXPECT_FALSE(sample.has_value());
}

TEST(SamplerTest, WeightedReservoirSample) {
  Sampler<double> sampler(strategies::weightedReservoirSample<double>);

  for (int i = 0; i < 1000; ++i) {
    sampler.yield(static_cast<double>(i));
  }

  auto sample = sampler.capture(nowUs());

  ASSERT_TRUE(sample.has_value());
  EXPECT_GT(sample->count, 0);
  EXPECT_GE(sample->value, 0.0);
  EXPECT_LT(sample->value, 1000.0);
}

TEST(SamplerTest, P99Sample) {
  Sampler<double> sampler(strategies::p99Sample<double>);

  for (int i = 0; i < 100; ++i) {
    sampler.yield(static_cast<double>(i));
  }

  auto sample = sampler.capture(nowUs());

  ASSERT_TRUE(sample.has_value());
  EXPECT_EQ(sample->count, 100);
  EXPECT_GE(sample->value, 98.0);
}

TEST(SamplerTest, AvgSample) {
  Sampler<double> sampler(strategies::avgSample<double>);

  sampler.yield(10.0);
  sampler.yield(20.0);
  sampler.yield(30.0);

  auto sample = sampler.capture(nowUs());

  ASSERT_TRUE(sample.has_value());
  EXPECT_EQ(sample->count, 3);
  EXPECT_DOUBLE_EQ(sample->value, 20.0);
}

TEST(SamplerTest, MinMaxSample) {
  Sampler<double> min_sampler(strategies::minSample<double>);
  Sampler<double> max_sampler(strategies::maxSample<double>);

  min_sampler.yield(5.0);
  min_sampler.yield(1.0);
  min_sampler.yield(10.0);

  max_sampler.yield(5.0);
  max_sampler.yield(1.0);
  max_sampler.yield(10.0);

  auto min_sample = min_sampler.capture(nowUs());
  auto max_sample = max_sampler.capture(nowUs());

  ASSERT_TRUE(min_sample.has_value());
  ASSERT_TRUE(max_sample.has_value());

  EXPECT_DOUBLE_EQ(min_sample->value, 1.0);
  EXPECT_DOUBLE_EQ(max_sample->value, 10.0);
}

TEST(SamplerTest, MultipleCapturesResetBuffer) {
  Sampler<double> sampler(strategies::avgSample<double>);

  sampler.yield(10.0);
  auto sample1 = sampler.capture(nowUs());
  ASSERT_TRUE(sample1.has_value());
  EXPECT_DOUBLE_EQ(sample1->value, 10.0);
  EXPECT_EQ(sample1->count, 1);

  sampler.yield(20.0);
  sampler.yield(30.0);
  auto sample2 = sampler.capture(nowUs());
  ASSERT_TRUE(sample2.has_value());
  EXPECT_DOUBLE_EQ(sample2->value, 25.0);
  EXPECT_EQ(sample2->count, 2);
}

TEST(SamplerTest, CustomSamplingStrategy) {
  auto custom_strategy = [](std::span<const double> values,
                            int64_t timestamp_us) -> Sample<double> {
    double sum = 0.0;
    for (const auto& v : values) {
      sum += v;
    }
    return {.value = sum,
            .timestamp_us = timestamp_us,
            .count = static_cast<uint64_t>(values.size())};
  };

  Sampler<double> sampler(std::move(custom_strategy));

  sampler.yield(1.0);
  sampler.yield(2.0);
  sampler.yield(3.0);

  auto sample = sampler.capture(nowUs());

  ASSERT_TRUE(sample.has_value());
  EXPECT_DOUBLE_EQ(sample->value, 6.0);
  EXPECT_EQ(sample->count, 3);
}
