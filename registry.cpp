#include "registry.hpp"
#include <mutex>
#include <vector>
#include "counter.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl::detail {

Registry* Registry::instance() {
  static Registry s;
  return &s;
}

template <typename T>
std::shared_ptr<T> Registry::makeObject(std::string name) {
  auto* s = instance();

  {
    std::shared_lock lock(s->obj_mutex);
    auto it = s->obj.find(name);
    if (it != s->obj.end()) {
      return std::static_pointer_cast<T>(it->second);
    }
  }

  {
    std::unique_lock lock(s->obj_mutex);
    auto it = s->obj.find(name);
    if (it != s->obj.end()) {
      return std::static_pointer_cast<T>(it->second);
    }

    auto impl = std::make_shared<T>(name);
    s->obj[name] = impl;
    return impl;
  }
}

std::vector<ITelemetryObjectPtr> Registry::getObjects() {
  auto* s = instance();
  std::vector<ITelemetryObjectPtr> snapshot;

  {
    std::shared_lock lock(s->obj_mutex);
    snapshot.reserve(s->obj.size());
    for (const auto& [name, obj] : s->obj) {
      snapshot.push_back(obj);
    }
  }

  return snapshot;
}

template std::shared_ptr<bits::ttl::Counter::Impl>
Registry::makeObject<bits::ttl::Counter::Impl>(std::string name);

}  // namespace bits::ttl::detail
