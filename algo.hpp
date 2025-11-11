#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <random>
#include <span>
#include <vector>

namespace bits {

// Reservoir Sampling:
// https://en.wikipedia.org/wiki/Reservoir_sampling
//
// +------------------------------+------------+----------+----------+
// | Use case                      | confidence | epsilon  | samples |
// +------------------------------+------------+----------+----------+
// | SLOs                          | 0.99       | 0.01     | ~26500  |
// | Production metrics (default)  | 0.95       | 0.05     | ~738    |
// | Development/debugging         | 0.90       | 0.10     | ~150    |
// | Rough trends/dashboards       | 0.90       | 0.20     | ~38     |
// +------------------------------+------------+----------+----------+
//

template <typename T, double Confidence = 0.95, double Epsilon = 0.05>
  requires std::is_arithmetic_v<T>
struct WeightedReservoirSample {
  struct Result {
    std::vector<T> samples;
    size_t original_count;
  };

  constexpr size_t sampleSize() {
    // For high percentiles, use Hoeffding's inequality:
    // P(|p_hat - p| > epsilon) <= 2 * exp(-2 * n * epsilon^2)
    // Setting this <= (1 - confidence) and solving for n:
    // n >= -ln((1 - confidence) / 2) / (2 * epsilon^2)

    const double kAlpha       = 1.0 - Confidence;
    const double kNumerator   = -std::log(kAlpha / 2.0);
    const double kDenominator = 2.0 * Epsilon * Epsilon;
    return static_cast<size_t>(std::ceil(kNumerator / kDenominator));
    ;
  }

  Result operator()(const std::span<const T>& values) {
    static thread_local std::mt19937 rng{std::random_device{}()};

    std::vector<T> reservoir;
    reservoir.reserve(sampleSize());

    for (size_t i = 0; i < values.size(); ++i) {
      if (i < sampleSize()) {
        reservoir.push_back(values[i]);
      } else {
        std::uniform_int_distribution<size_t> dist(0, i);
        size_t j = dist(rng);
        if (j < sampleSize()) {
          reservoir[j] = values[i];
        }
      }
    }

    return {std::move(reservoir), values.size()};
  }
};

template <size_t Quantile, size_t Buckets = 100, size_t Precision = 1000,
          double MaxValue = 1e6, double LogBase = 10.0>
struct Histogram {
  Histogram()                            = default;
  Histogram(const Histogram&)            = delete;
  Histogram(Histogram&&)                 = delete;
  Histogram& operator=(const Histogram&) = delete;
  Histogram& operator=(Histogram&&)      = delete;

  double operator()(const std::span<double>& values) {
    buckets_.fill(0);
    count_ = 0;
    max_   = std::numeric_limits<double>::min();

    for (const auto& value : values) {
      auto bucket = getBucket(value);
      buckets_[bucket]++;
      count_++;
      max_ = std::max(max_, value);
    }

    if (count_ == 0) {
      return 0;
    }

    auto target =
        static_cast<size_t>(static_cast<double>(count_) * Quantile / Buckets);
    size_t sum = 0;

    for (size_t i = 0; i < Precision; ++i) {
      sum += buckets_[i];
      if (sum >= target) {
        return getBucketValue(i);
      }
    }
    return max_;
  }

 private:
  [[nodiscard]] static size_t getBucket(double value) {
    if (value <= 0) {
      return 0;
    }

    if (value >= MaxValue) {
      return Precision - 1;
    }

    double log_value = std::log(value + 1) / std::numbers::ln10;
    double log_max   = std::log(MaxValue + 1) / std::numbers::ln10;
    auto bucket      = static_cast<size_t>((log_value / log_max) *
                                           static_cast<double>(Precision - 1));
    return std::min(bucket, Precision - 1);
  }

  [[nodiscard]] static double getBucketValue(size_t bucket) {
    double log_max   = std::log(MaxValue + 1) / std::numbers::ln10;
    double log_value = (static_cast<double>(bucket) * log_max) /
                       static_cast<double>(Precision - 1);
    return std::pow(LogBase, log_value) - 1;
  }

  size_t count_ = 0;
  double max_   = 0;
  std::array<size_t, Precision> buckets_{};
};

}  // namespace bits
