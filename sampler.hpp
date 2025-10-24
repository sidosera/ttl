#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace bits::ttl {

constexpr size_t kShards = 1 << 6;
static_assert((kShards & (kShards - 1)) == 0);

constexpr size_t kBufferCapacity = 1 << 9;
static_assert((kBufferCapacity & (kBufferCapacity - 1)) == 0);

template <typename T>
struct alignas(64) Buffer {
  std::array<T, kBufferCapacity> values{};
  std::atomic<size_t> offset{0};
};

template <typename T>
struct Shard {
  Buffer<T> first{};
  Buffer<T> second{};
  std::atomic<Buffer<T>*> current{&first};

  Buffer<T>* swap() {
    Buffer<T>* previous = current.load(std::memory_order_acquire);
    Buffer<T>* next;
    do {
      next = (previous == &first) ? &second : &first;
    } while (!current.compare_exchange_weak(
        previous, next, std::memory_order_acq_rel, std::memory_order_acquire));
    return previous;
  }

  void yield(const T& value) {
    auto* shard_q = current.load(std::memory_order_relaxed);
    auto slot = shard_q->offset.fetch_add(1, std::memory_order_relaxed);
    shard_q->values[slot & (kBufferCapacity - 1)] = value;
  }
};

template <typename T>
struct Sample {
  T value;
  int64_t timestamp_us;
  uint64_t count;
};

template <typename T>
using SamplingStrategy =
    std::move_only_function<Sample<T>(std::span<const T>, int64_t)>;

template <typename T>
class Sampler {
 public:
  explicit Sampler(SamplingStrategy<T> strategy)
      : strategy_(std::move(strategy)) {}

  void yield(const T& value) {
    auto& shard = shards_[shard_id_];
    shard.yield(value);
  }

  std::optional<Sample<T>> capture(int64_t now_us) {
    std::vector<T> all_values;
    uint64_t total_count = 0;

    for (auto& shard : shards_) {
      auto* prev = shard.swap();
      if (!prev) {
        continue;
      }

      const auto offset = prev->offset.exchange(0, std::memory_order_acq_rel);
      if (offset == 0) {
        continue;
      }

      const auto limit = std::min(offset, kBufferCapacity);
      total_count += limit;
      all_values.insert(all_values.end(), prev->values.begin(),
                        prev->values.begin() + limit);
    }

    if (total_count == 0) {
      return std::nullopt;
    }

    auto sample = strategy_(std::span<const T>(all_values), now_us);
    sample.count = total_count;
    return sample;
  }

 private:
  static inline thread_local size_t shard_id_ = [] {
    static std::atomic<size_t> counter{0};
    return counter.fetch_add(1, std::memory_order_relaxed) & (kShards - 1);
  }();

  SamplingStrategy<T> strategy_;
  mutable std::array<Shard<T>, kShards> shards_{};
};

}  // namespace bits::ttl
