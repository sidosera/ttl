#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include "measure.hpp"
#include "sink.hpp"

namespace bits::ttl {

struct TtlConfig {
  Sink& sink;
  Tag tags[8];
  size_t tag_count{0};
  int flush_interval_seconds{1};
};

class TtlFactory {
 public:
  explicit TtlFactory(const TtlConfig& config);
  ~TtlFactory();

  TtlFactory(const TtlFactory&) = delete;
  TtlFactory& operator=(const TtlFactory&) = delete;

  TtlFactory(TtlFactory&&) = delete;
  TtlFactory& operator=(TtlFactory&&) = delete;

  Measure& measure(std::string_view name);

 private:
  void doFlush();

  TtlConfig config_;
  std::unordered_map<std::string, std::unique_ptr<Measure>> measures_;
  mutable std::mutex measures_mutex_;

  std::atomic<bool> running_{true};
  std::thread flush_thread_;
};

}  // namespace bits::ttl
