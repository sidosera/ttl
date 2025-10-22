#include "measure.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include "factory.hpp"
#include "sink.hpp"

namespace bits::ttl {

Measure::Measure(std::string_view name)
    : name_(name){}

size_t Measure::segmentIndex() const {
  thread_local size_t shard = [] {
    static std::atomic<size_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed) & (kSegments - 1);
  }();
  return shard;
}

Buffer* Segment::flip() {
  Buffer* previous = current.load(std::memory_order_acquire);
  Buffer* next;
  do {
    next = (previous == &first) ? &second : &first;
  } while (!current.compare_exchange_weak(
      previous, next, std::memory_order_acq_rel, std::memory_order_acquire));
  return previous;
}

void Measure::record(double value) {
  thread_local size_t shard_id = [] {
    static std::atomic<size_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed) & (kSegments - 1);
  }();

  auto& shard = shards_[shard_id];
  auto* shard_q = shard.current.load(std::memory_order_relaxed);
  auto slot = shard_q->write_index.fetch_add(1, std::memory_order_relaxed);
  shard_q->values[slot & (kBufferCapacity - 1)] = value;
}

void Measure::operator+=(double value) {
  record(value);
}

void Measure::operator=(double value) {
  record(value);
}

void Measure::operator()(double value) {
  record(value);
}

void Measure::grab(TtlConfig& s) const {
  double sum = 0.0;
  double min_val = std::numeric_limits<double>::max();
  double max_val = std::numeric_limits<double>::lowest();

  size_t sample_count = 0;
  welford_mean_ = 0.0;
  welford_m2_ = 0.0;
  hist_underflow_ = 0;
  hist_overflow_ = 0;
  std::fill(hist_counts_.begin(), hist_counts_.end(), 0);

  for (auto& seg : shards_) {
    Buffer* drain = seg.flip();

    const size_t written =
        drain->write_index.exchange(0, std::memory_order_acq_rel);

    if (written > 0) {
      const size_t consumed =
          written > kBufferCapacity ? kBufferCapacity : written;
      const size_t start = written > kBufferCapacity ? written : 0;

      for (size_t i = 0; i < consumed; ++i) {
        const size_t idx = (start + i) % kBufferCapacity;
        const double v = drain->values[idx];
        scratch_[sample_count++] = v;
        sum += v;
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;

        const double delta = v - welford_mean_;
        welford_mean_ += delta / sample_count;
        welford_m2_ += delta * (v - welford_mean_);

        if (v < kHistStart) {
          hist_underflow_++;
        } else {
          const double hi = kHistStart * std::pow(kHistFactor, kHistBins);
          if (v >= hi) {
            hist_overflow_++;
          } else {
            const size_t bin_idx =
                std::min(kHistBins - 1,
                         static_cast<size_t>(std::log(v / kHistStart) /
                                             std::log(kHistFactor)));
            hist_counts_[bin_idx]++;
          }
        }
      }
    }
  }

  const int64_t count = static_cast<int64_t>(sample_count);

  Field fields[8];
  size_t field_count = 0;

  auto add_field = [&](std::string_view key, Value value) {
    if (field_count < 8) {
      fields[field_count++] = {key, value};
    }
  };

  add_field("count", count);

  if (count > 0) {
    add_field("sum", sum);
    add_field("min", min_val);
    add_field("max", max_val);
    add_field("mean", welford_mean_);
    add_field("M2", welford_m2_);
  }

  Histogram hist{
      .scale = HistScale::Log,
      .bins = static_cast<uint16_t>(kHistBins),
      .start = kHistStart,
      .step = 0.0,
      .factor = kHistFactor,
      .underflow = hist_underflow_,
      .counts = hist_counts_.data(),
      .overflow = hist_overflow_,
  };

  Event event{
      .name = name(),
      .timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count(),
      .tags = s.tags,
      .tag_count = s.tag_count,
      .fields = fields,
      .field_count = field_count,
      .histogram = &hist,
  };

  s.sink.publish(event);
}

}  // namespace bits::ttl
