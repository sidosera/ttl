#pragma once

#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "sink.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl {
class Ttl;
}

namespace bits::ttl::detail {

class Runtime {
 public:
  Runtime() = default;
  ~Runtime();

  Runtime(const Runtime&) = delete;
  Runtime& operator=(const Runtime&) = delete;
  Runtime(Runtime&&) = delete;
  Runtime& operator=(Runtime&&) = delete;

  static std::shared_ptr<Runtime> instance();

  void init(std::unique_ptr<ISink> sink);
  void shutdown();

  template <typename T>
  std::shared_ptr<T> makeObject(const std::string& name);

  std::vector<ITelemetryObjectPtr> getObjects();

 private:
  std::shared_mutex mutex_;
  std::unordered_map<std::string, ITelemetryObjectPtr> obj_;

  std::unique_ptr<ISink> sink_;

  std::unique_ptr<std::jthread> flush_thread_;
};
}  // namespace bits::ttl::detail
