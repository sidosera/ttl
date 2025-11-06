#pragma once

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include "sink.hpp"
#include "telemetry_object.hpp"
#include "types.hpp"

namespace bits::ttl {
class Ttl;
}

namespace bits::ttl::detail {

class Registry {
 public:
  template <typename T>
  static std::shared_ptr<T> makeObject(std::string name);

  static std::vector<ITelemetryObjectPtr> getObjects();

 private:
  friend class bits::ttl::Ttl;

  std::unique_ptr<Sink> sink;
  std::vector<Tag> global_tags;
  std::shared_mutex obj_mutex;
  std::unordered_map<std::string, ITelemetryObjectPtr> obj;

  std::atomic<bool> running{false};
  std::unique_ptr<std::thread> collector_thread;
  int flush_interval_ms{100};

  static Registry* instance();
};
}  // namespace bits::ttl::detail
