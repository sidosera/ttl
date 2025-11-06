#include "counter.hpp"
#include <chrono>
#include "registry.hpp"
#include "sampling_strategies.hpp"
#include "sink.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl {

Counter::Impl::Impl(std::string_view name)
    : name_(name), sampler_(strategies::weightedReservoirSample<double>) {}

void Counter::Impl::yield(double value) {
  sampler_.yield(value);
}

void Counter::Impl::capture(Sink& sink, const std::vector<Tag>& tags) {
  int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();

  auto sample_opt = sampler_.capture(now_us);
  if (!sample_opt) {
    return;
  }

  const auto& sample = *sample_opt;

  Event event{.name = std::string(name()),
              .timestamp = sample.timestamp_us / 1000000,
              .tags = tags,
              .fields = {{"value", sample.value},
                         {"count", static_cast<int64_t>(sample.count)}}};

  sink.publish(std::move(event));
}

Counter::Counter(std::string_view name)
    : impl_(detail::Registry::makeObject<Impl>(std::string(name))) {}

Counter& Counter::operator+=(double value) {
  impl_->yield(value);
  return *this;
}

Counter& Counter::operator=(double value) {
  impl_->yield(value);
  return *this;
}

void Counter::operator()(double value) {
  impl_->yield(value);
}

std::string_view Counter::name() const {
  return impl_->name();
}

}  // namespace bits::ttl
