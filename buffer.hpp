#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <span>
#include <vector>

namespace bits {

constexpr size_t kSlots = (1 << 10);
constexpr size_t kMask  = kSlots - 1;

template <typename T>
struct alignas(64) Wrap {
  Wrap()                       = default;
  Wrap(const Wrap&)            = delete;
  Wrap& operator=(const Wrap&) = delete;

  void append(const T& v) noexcept {
    const size_t idx = write_.fetch_add(1, std::memory_order_release) & kMask;
    values_[idx]     = v;
  }

  size_t drainInto(std::vector<T>& out) noexcept {
    const size_t wrote = write_.exchange(0, std::memory_order_acq_rel);
    if (wrote == 0U) {
      return 0;
    }
    const size_t n     = std::min<size_t>(wrote, kSlots);
    const size_t end   = (wrote - 1) & kMask;
    const size_t start = (wrote - n) & kMask;
    if (start <= end) {
      out.insert(out.end(), values_.begin() + start, values_.begin() + end + 1);
    } else {
      out.insert(out.end(), values_.begin() + start, values_.end());
      out.insert(out.end(), values_.begin(), values_.begin() + end + 1);
    }
    return n;
  }

  std::array<T, kSlots> values_{};
  std::atomic<size_t> write_{0};
};

template <typename T>
struct alignas(64) Shard {
  Wrap<T> buffers_[2];
  std::atomic<unsigned> current_{0};

  void append(const T& v) noexcept {
    const unsigned curr = current_.load(std::memory_order_acquire) & 1U;
    buffers_[curr].append(v);
  }

  void flip(std::vector<T>& out) noexcept {
    const unsigned prev =
        current_.fetch_xor(1U, std::memory_order_acq_rel) & 1U;
    auto& old = buffers_[prev];
    old.drainInto(out);
  }
};

template <typename T, size_t Shards = 64>
class MPSCBuffer {
 public:
  MPSCBuffer() { scratch_.reserve(Shards * kSlots); }

  MPSCBuffer(const MPSCBuffer&)            = delete;
  MPSCBuffer& operator=(const MPSCBuffer&) = delete;

  void append(const T& v) noexcept {
    const auto& idx = shard_id_ & (Shards - 1);
    shards_[idx].append(v);
  }

  std::span<const T> acquire() noexcept {
    scratch_.clear();
    for (auto& s : shards_) {
      s.flip(scratch_);
    }
    return {scratch_.data(), scratch_.size()};
  }

 private:
  static inline thread_local size_t shard_id_ = [] {
    static std::atomic<size_t> ctr{0};
    return ctr.fetch_add(1, std::memory_order_relaxed);
  }();

  std::vector<T> scratch_;
  std::array<Shard<T>, Shards> shards_;
};

}  // namespace bits
