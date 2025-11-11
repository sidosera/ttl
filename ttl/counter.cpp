#include "counter.hpp"
#include <bits/algo.hpp>
#include <chrono>
#include <memory>
#include <string>
#include "runtime.hpp"
#include "sink.hpp"
#include "telemetry_object.hpp"

using std::chrono::steady_clock;

namespace bits::ttl {

namespace detail {
CounterImpl::CounterImpl(std::string name) : name_(std::move(name)) {}

void CounterImpl::add(double value) {
  buffer_.append(value);
}

void CounterImpl::capture(ISink& sink) {
  bits::WeightedReservoirSample<double> sampler;
  const auto data = buffer_.acquire();

  if (data.empty()) {
    return;
  }

  const auto result = sampler(data);

  if (result.samples.empty()) {
    return;
  }

  for (const auto& value : result.samples) {
    Event event{
        .type      = "metric",
        .name      = std::string(name()),
        .timestamp = steady_clock::now().time_since_epoch(),
        .fields    = {{"value", value}, {"count", static_cast<int64_t>(result.original_count)}}};

    sink.publish(std::move(event));
  }
}
}  // namespace detail

Counter::Counter(std::string_view name)
    : Counter(name, detail::Runtime::instance()) {};

Counter::Counter(std::string_view name,
                 const std::shared_ptr<detail::Runtime>& rt)
    : impl_(rt->makeObject<detail::CounterImpl>(std::string(name))) {}

Counter& Counter::operator+=(double value) {
  impl_->add(value);
  return *this;
}

Counter& Counter::operator=(double value) {
  impl_->add(value);
  return *this;
}

void Counter::operator()(double value) {
  impl_->add(value);
}

std::string_view Counter::name() const {
  return impl_->name();
}

}  // namespace bits::ttl
