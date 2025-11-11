#pragma once

#include <memory>
#include <string>
#include <string_view>
#include "buffer.hpp"
#include "runtime.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl {

namespace detail {
struct CounterImpl : public ITelemetryObject {
  explicit CounterImpl(std::string name);

  void add(double value);
  void capture(ISink& sink) override;

  [[nodiscard]] std::string_view name() const { return name_; }

  std::string name_;
  bits::MPSCBuffer<double> buffer_;
};
}  // namespace detail

class Counter {
 public:
  explicit Counter(std::string_view name);
  explicit Counter(std::string_view name,
                   const std::shared_ptr<detail::Runtime>& rt);

  Counter& operator+=(double value);
  Counter& operator=(double value);
  void operator()(double value);

  [[nodiscard]] std::string_view name() const;

 private:
  std::shared_ptr<detail::CounterImpl> impl_;
};

}  // namespace bits::ttl
