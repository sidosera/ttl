#include "factory.hpp"
#include <atomic>
#include <chrono>

using namespace std::chrono;

namespace bits::ttl {

TtlFactory::TtlFactory(const TtlConfig& config) : config_(config) {
  flush_thread_ = std::thread([this] {
    auto next = steady_clock::now() + seconds(config_.flush_interval_seconds);
    while (running_.load(std::memory_order_relaxed)) {
      auto now = steady_clock::now();
      if (now < next) {
        std::this_thread::sleep_until(next);
      }
      doFlush();
      next += seconds(config_.flush_interval_seconds);
    }
  });
}

TtlFactory::~TtlFactory() {
  if (running_.exchange(false, std::memory_order_relaxed)) {
    if (flush_thread_.joinable()) {
      flush_thread_.join();
    }
  }
}

Measure& TtlFactory::measure(std::string_view name) {
  std::lock_guard<std::mutex> lock(measures_mutex_);
  auto [it, inserted] = measures_.try_emplace(std::string(name), nullptr);
  if (inserted) {
    it->second = std::make_unique<Measure>(name);
  }
  return *it->second;
}

void TtlFactory::doFlush() {
  std::lock_guard<std::mutex> lock(measures_mutex_);
  for (auto& [name, measure] : measures_) {
    measure->grab(config_);
  }
}

}  // namespace bits::ttl
