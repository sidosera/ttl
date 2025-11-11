#include <gtest/gtest.h>
#include <bits/algo.hpp>
#include <algorithm>
#include <vector>

using namespace bits;

TEST(WeightedReservoirSampleTest, ConfidenceBounds) {
  // Test with looser bounds that don't hit the max
  // Using 0.20 epsilon (20% error) to get meaningful differences
  WeightedReservoirSample<double, 0.90, 0.20> sampler_90;
  WeightedReservoirSample<double, 0.95, 0.20> sampler_95;
  WeightedReservoirSample<double, 0.99, 0.20> sampler_99;

  size_t size_90 = sampler_90.sampleSize();
  size_t size_95 = sampler_95.sampleSize();
  size_t size_99 = sampler_99.sampleSize();

  // Higher confidence should require larger samples
  EXPECT_LT(size_90, size_95);
  EXPECT_LT(size_95, size_99);

  // Smaller epsilon (tighter error bound) should require larger samples
  WeightedReservoirSample<double, 0.90, 0.15> sampler_eps_15;
  WeightedReservoirSample<double, 0.90, 0.20> sampler_eps_20;

  size_t size_eps_15 = sampler_eps_15.sampleSize();
  size_t size_eps_20 = sampler_eps_20.sampleSize();
  EXPECT_GT(size_eps_15, size_eps_20);

  // Check expected sample sizes (allow ±1 for rounding)
  WeightedReservoirSample<double, 0.95, 0.05> sampler_default;
  EXPECT_NEAR(sampler_default.sampleSize(), 737, 1);

  WeightedReservoirSample<double, 0.99, 0.01> sampler_slo;
  EXPECT_NEAR(sampler_slo.sampleSize(), 26492, 1);
}

TEST(WeightedReservoirSampleTest, PreservesDistribution) {
  WeightedReservoirSample<double> sampler;

  // Create a large dataset with known distribution
  std::vector<double> data;
  data.reserve(10000);
  for (int i = 0; i < 10000; i++) {
    data.push_back(static_cast<double>(i));
  }

  // Sample with default parameters
  auto result = sampler(data);

  EXPECT_GT(result.samples.size(), 0);
  EXPECT_NEAR(result.samples.size(), 737, 1);
  EXPECT_EQ(result.original_count, 10000);

  // Verify samples are within the original range
  for (const auto& sample : result.samples) {
    EXPECT_GE(sample, 0.0);
    EXPECT_LT(sample, 10000.0);
  }

  // Compute mean of samples (should be close to 4999.5)
  double sum = 0.0;
  for (const auto& sample : result.samples) {
    sum += sample;
  }
  double mean = sum / static_cast<double>(result.samples.size());

  // Mean should be within reasonable bounds
  EXPECT_GT(mean, 4000.0);
  EXPECT_LT(mean, 6000.0);
}

TEST(WeightedReservoirSampleTest, SmallDatasets) {
  WeightedReservoirSample<double> sampler;

  // Test with data smaller than reservoir size
  std::vector<double> small_data = {1.0, 2.0, 3.0, 4.0, 5.0};
  auto result = sampler(small_data);

  // Should return all values when dataset is small
  EXPECT_EQ(result.samples.size(), 5);
  EXPECT_EQ(result.original_count, 5);

  // All original values should be present
  std::vector<double> sorted_samples = result.samples;
  std::ranges::sort(sorted_samples);
  for (size_t i = 0; i < small_data.size(); i++) {
    EXPECT_DOUBLE_EQ(sorted_samples[i], small_data[i]);
  }
}

TEST(WeightedReservoirSampleTest, EmptyDataset) {
  WeightedReservoirSample<double> sampler;

  std::vector<double> empty_data;
  auto result = sampler(empty_data);

  EXPECT_EQ(result.samples.size(), 0);
  EXPECT_EQ(result.original_count, 0);
}

TEST(WeightedReservoirSampleTest, HighConfidenceRequiresLargeSample) {
  WeightedReservoirSample<double, 0.99, 0.01> sampler_slo;

  // 99% confidence with 1% error should require a large sample
  size_t size = sampler_slo.sampleSize();

  // Based on Hoeffding's inequality:
  // n >= -ln(0.01/2) / (2 * 0.01^2) = -ln(0.005) / 0.0002 ≈ 26492
  EXPECT_EQ(size, 26492);
}

TEST(WeightedReservoirSampleTest, ReasonableConfidenceGivesUsefulSize) {
  WeightedReservoirSample<double, 0.95, 0.05> sampler_prod;

  // 95% confidence with 5% error should give a reasonable sample size
  size_t size = sampler_prod.sampleSize();

  // Based on Hoeffding's inequality:
  // n >= -ln(0.05/2) / (2 * 0.05^2) = -ln(0.025) / 0.005 ≈ 737
  EXPECT_NEAR(size, 737, 1);

  // With looser epsilon, we get smaller size
  WeightedReservoirSample<double, 0.95, 0.10> sampler_loose;
  size_t size_loose = sampler_loose.sampleSize();
  // n >= -ln(0.025) / (2 * 0.10^2) = -ln(0.025) / 0.02 ≈ 184
  EXPECT_LT(size_loose, size);
  EXPECT_GT(size_loose, 100);
}

TEST(WeightedReservoirSampleTest, DefaultParameters) {
  WeightedReservoirSample<double> sampler;

  std::vector<double> data;
  data.reserve(1000);
  for (int i = 0; i < 1000; i++) {
    data.push_back(static_cast<double>(i));
  }

  // Call with defaults (0.95 confidence, 0.05 epsilon)
  auto result = sampler(data);

  EXPECT_GT(result.samples.size(), 0);
  EXPECT_NEAR(result.samples.size(), 737, 1);
  EXPECT_EQ(result.original_count, 1000);
}

TEST(WeightedReservoirSampleTest, DifferentConfigurations) {
  // Test the documented configurations from the table
  WeightedReservoirSample<double, 0.99, 0.01> slo;
  WeightedReservoirSample<double, 0.95, 0.05> prod;
  WeightedReservoirSample<double, 0.90, 0.10> dev;
  WeightedReservoirSample<double, 0.90, 0.20> dashboard;

  EXPECT_NEAR(slo.sampleSize(), 26492, 1);
  EXPECT_NEAR(prod.sampleSize(), 738, 1);
  EXPECT_NEAR(dev.sampleSize(), 150, 1);
  EXPECT_NEAR(dashboard.sampleSize(), 38, 1);
}
