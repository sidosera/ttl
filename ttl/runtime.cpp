#include "runtime.hpp"
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>
#include "counter.hpp"
#include "logger.hpp"
#include "sink.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl::detail {
using std::chrono::milliseconds;

std::shared_ptr<Runtime> Runtime::instance() {
  static std::shared_ptr<Runtime> s{new Runtime()};
  return s;
}

Runtime::~Runtime() {
  shutdown();
}

void Runtime::init(std::unique_ptr<ISink> sink) {
  if (flush_thread_) {
    throw std::runtime_error("Runtime already initialized. Call shutdown() first.");
  }

  this->sink_         = std::move(sink);
  this->flush_thread_ = std::make_unique<std::jthread>(
      [this](const std::stop_token& token) {
        std::mutex mutex;
        std::condition_variable cond;
        std::stop_callback callback(token, [&] { cond.notify_one(); });

        while (!token.stop_requested()) {
          for (auto& obj : getObjects()) {
            obj->capture(*this->sink_);
          }

          const auto deadline =
              std::chrono::steady_clock::now() + milliseconds(100);

          std::unique_lock lock(mutex);
          cond.wait_until(lock, deadline);
        }

        // Final flush before thread exits
        for (auto& obj : getObjects()) {
          obj->capture(*this->sink_);
        }
      });
}

void Runtime::shutdown() {
  if (flush_thread_ && flush_thread_->joinable() && flush_thread_) {
    flush_thread_->request_stop();
    flush_thread_->join();
    flush_thread_ = nullptr;
  }
}

template <typename T>
std::shared_ptr<T> Runtime::makeObject(const std::string& name) {
  {
    std::shared_lock lock(mutex_);
    const auto& it = obj_.find(name);
    if (it != obj_.end()) {
      return std::static_pointer_cast<T>(it->second);
    }
  }

  {
    std::unique_lock lock(mutex_);
    const auto& it = obj_.find(name);
    if (it != obj_.end()) {
      return std::static_pointer_cast<T>(it->second);
    }

    auto impl  = std::make_shared<T>(name);
    obj_[name] = impl;
    return impl;
  }
}

std::vector<ITelemetryObjectPtr> Runtime::getObjects() {
  std::vector<ITelemetryObjectPtr> copy;
  {
    std::shared_lock lock(mutex_);
    copy.reserve(obj_.size());
    for (const auto& [name, obj] : obj_) {
      copy.push_back(obj);
    }
  }

  return copy;
}

template std::shared_ptr<bits::ttl::detail::CounterImpl>
Runtime::makeObject<bits::ttl::detail::CounterImpl>(const std::string& name);

template std::shared_ptr<bits::ttl::detail::LoggerImpl>
Runtime::makeObject<bits::ttl::detail::LoggerImpl>(const std::string& name);

}  // namespace bits::ttl::detail
