#pragma once

#include <atomic>
#include <optional>

namespace bits {

// D. Vyukov Multi-Producer/Single-Consumer queue
// http://bit.ly/4qi2pGR
template <typename T>
class MPSCQueue {
 public:
  MPSCQueue() {
    head_.store(&stub_, std::memory_order_relaxed);
    tail_.store(&stub_, std::memory_order_relaxed);
    stub_.next.store(nullptr, std::memory_order_relaxed);
  }

  ~MPSCQueue() {
    while (tryTake()) {}
  }

  // MP (Multi-Producer)
  void push(T value) {
    Node* node = new Node(std::move(value));
    node->next.store(nullptr, std::memory_order_relaxed);

    Node* prev = head_.exchange(node, std::memory_order_acq_rel);
    prev->next.store(node, std::memory_order_release);
  }

  // CS (Single-Consumer)
  std::optional<T> tryTake() {
    while (true) {
      Node* tail = tail_.load(std::memory_order_relaxed);
      Node* next = tail->next.load(std::memory_order_acquire);

      if (tail == &stub_) {
        if (next == nullptr) {
          Node* head = head_.load(std::memory_order_acquire);
          if (tail != head) {
            continue;
          }
          return std::nullopt;
        }

        tail_.store(next, std::memory_order_relaxed);
        tail = next;
        next = tail->next.load(std::memory_order_acquire);
      }

      if (next != nullptr) {
        tail_.store(next, std::memory_order_relaxed);
        T value = std::move(tail->value);
        delete tail;
        return value;
      }

      Node* head = head_.load(std::memory_order_acquire);
      if (tail != head) {
        continue;
      }

      push_stub();

      next = tail->next.load(std::memory_order_acquire);
      if (next != nullptr) {
        tail_.store(next, std::memory_order_relaxed);
        T value = std::move(tail->value);
        delete tail;
        return value;
      }

      continue;
    }
  }

 private:
  struct Node {
    T value;
    std::atomic<Node*> next;

    explicit Node() : value{}, next{nullptr} {}
    explicit Node(T v) : value{std::move(v)}, next{nullptr} {}
  };

  void push_stub() {
    stub_.next.store(nullptr, std::memory_order_relaxed);
    Node* prev = head_.exchange(&stub_, std::memory_order_acq_rel);
    prev->next.store(&stub_, std::memory_order_release);
  }

  std::atomic<Node*> head_;
  std::atomic<Node*> tail_;
  Node stub_;
};

}  // namespace bits
