#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <vector>
#include "sampler.hpp"

namespace bits::ttl::strategies {

template <typename T>
Sample<T> uniformSample(std::span<const T> values, int64_t timestamp_us) {
  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<size_t> dist(0, values.size() - 1);
  return {.value = values[dist(rng)],
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

template <typename T>
  requires std::is_arithmetic_v<T>
Sample<T> weightedReservoirSample(std::span<const T> values,
                                   int64_t timestamp_us) {
  static thread_local std::mt19937 rng{std::random_device{}()};

  constexpr size_t kReservoirSize = 256;
  std::vector<T> reservoir;
  reservoir.reserve(std::min(values.size(), kReservoirSize));

  for (size_t i = 0; i < values.size(); ++i) {
    if (i < kReservoirSize) {
      reservoir.push_back(values[i]);
    } else {
      std::uniform_int_distribution<size_t> dist(0, i);
      size_t j = dist(rng);
      if (j < kReservoirSize) {
        reservoir[j] = values[i];
      }
    }
  }

  if (reservoir.empty()) {
    return {.value = T{}, .timestamp_us = timestamp_us, .count = 0};
  }

  std::uniform_int_distribution<size_t> dist(0, reservoir.size() - 1);
  return {.value = reservoir[dist(rng)],
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

template <typename T>
  requires std::is_arithmetic_v<T>
Sample<T> p99Sample(std::span<const T> values, int64_t timestamp_us) {
  std::vector<T> sorted(values.begin(), values.end());
  std::sort(sorted.begin(), sorted.end());
  size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
  T value = sorted[std::min(p99_idx, sorted.size() - 1)];
  return {.value = value,
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

template <typename T>
  requires std::is_arithmetic_v<T>
Sample<T> avgSample(std::span<const T> values, int64_t timestamp_us) {
  double sum = 0.0;
  for (const auto& v : values) {
    sum += static_cast<double>(v);
  }
  T avg = static_cast<T>(sum / values.size());
  return {.value = avg,
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

template <typename T>
  requires std::is_arithmetic_v<T>
Sample<T> minSample(std::span<const T> values, int64_t timestamp_us) {
  T min_val = *std::min_element(values.begin(), values.end());
  return {.value = min_val,
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

template <typename T>
  requires std::is_arithmetic_v<T>
Sample<T> maxSample(std::span<const T> values, int64_t timestamp_us) {
  T max_val = *std::max_element(values.begin(), values.end());
  return {.value = max_val,
          .timestamp_us = timestamp_us,
          .count = static_cast<uint64_t>(values.size())};
}

}  // namespace bits::ttl::strategies
