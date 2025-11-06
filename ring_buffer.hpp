#pragma once

#include <array>
#include <cstddef>
#include <utility>

namespace bits {

template <typename T, std::size_t N>
class RingBuffer {
 public:
  bool full() const noexcept { return size_ == N; }
  bool empty() const noexcept { return size_ == 0; }
  std::size_t size() const noexcept { return size_; }

  void push_back(T item) {
    data_[tail_] = std::move(item);
    tail_ = (tail_ + 1) % N;
    if (size_ < N) {
      ++size_;
    } else {
      head_ = (head_ + 1) % N;
    }
  }

  T& front() { return data_[head_]; }
  const T& front() const { return data_[head_]; }

  void pop_front() {
    if (size_ == 0) {
      return;
    }
    head_ = (head_ + 1) % N;
    --size_;
  }

  T& operator[](std::size_t i) { return data_[(head_ + i) % N]; }
  const T& operator[](std::size_t i) const { return data_[(head_ + i) % N]; }

 private:
  std::array<T, N> data_{};
  std::size_t head_{0};
  std::size_t tail_{0};
  std::size_t size_{0};
};

}  // namespace bits

