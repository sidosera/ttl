#include "ttl.hpp"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <format>
#include <mutex>
#include <stdexcept>
#include <thread>
#include "file_sink.hpp"
#include "noop_sink.hpp"
#include "registry.hpp"

namespace bits::ttl {

void Ttl::init(std::string_view url) {
  auto s = detail::Registry::instance();

  if (s->running.exchange(true, std::memory_order_acq_rel)) {
    return;
  }

  constexpr auto& p = "://";
  const auto& scheme_end = url.find(p);
  if (scheme_end == std::string_view::npos) {
    throw std::invalid_argument(
        std::format("invalid connection string: {}", url));
  }

  auto path = url.substr(scheme_end + std::strlen(p));
  auto scheme = url.substr(0, scheme_end);

  if (scheme == "file") {
    s->sink = std::make_unique<FileSink>(path);
  } else if (scheme == "noop") {
    s->sink = std::make_unique<NoOp>();
  } else {
    throw std::invalid_argument(std::format("unsupported scheme: {}", scheme));
  }

  s->collector_thread = std::make_unique<std::thread>([s]() {
    auto next = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(s->flush_interval_ms);

    while (s->running.load(std::memory_order_relaxed)) {
      std::this_thread::sleep_until(next);

      for (auto& obj : s->getObjects()) {
        obj->capture(*s->sink, s->global_tags);
      }

      next += std::chrono::milliseconds(s->flush_interval_ms);
    }
  });
}

void Ttl::shutdown() {
  auto s = detail::Registry::instance();

  if (!s->running.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  if (s->collector_thread && s->collector_thread->joinable()) {
    s->collector_thread->join();
  }

  for (auto& obj : s->getObjects()) {
    obj->capture(*s->sink, s->global_tags);
  }

  {
    std::unique_lock lock(s->obj_mutex);
    s->obj.clear();
  }
}

}  // namespace bits::ttl
