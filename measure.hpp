#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <string>
#include <string_view>
#include "types.hpp"

namespace bits::ttl {

constexpr size_t kSegments = 1 << 6;
static_assert((kSegments & (kSegments - 1)) == 0);

constexpr size_t kBufferCapacity = 1 << 9;
static_assert((kBufferCapacity & (kBufferCapacity - 1)) == 0);

constexpr size_t kMaxSamples = kSegments * kBufferCapacity;

constexpr size_t kHistBins = 128;
constexpr double kHistStart = 1e-6;
constexpr double kHistFactor = 1.2;

struct alignas(64) Buffer {
  std::array<double, kBufferCapacity> values{};
  std::atomic<size_t> write_index{0};
};

struct Segment {
  Buffer first{};
  Buffer second{};
  std::atomic<Buffer*> current{&first};

  Buffer* flip();
};

class Measure {
 public:
  Measure(std::string_view name);

  void operator+=(double value);
  void operator=(double value);
  void operator()(double value);

  std::string_view name() const { return name_; }

  void grab(TtlConfig& s) const;

 private:
  size_t segmentIndex() const;
  void record(double value);

  std::string name_;
  
  mutable std::array<Segment, kSegments> shards_{};
  mutable std::array<double, kMaxSamples> scratch_;
  mutable std::array<uint64_t, kHistBins> hist_counts_{};
  mutable uint64_t hist_underflow_{0};
  mutable uint64_t hist_overflow_{0};
  mutable double welford_mean_{0.0};
  mutable double welford_m2_{0.0};
};

}  // namespace bits::ttl
