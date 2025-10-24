#pragma once

#include <memory>
#include <string>
#include <string_view>
#include "sampler.hpp"
#include "telemetry_object.hpp"

namespace bits::ttl {

class Counter {
 public:
  explicit Counter(std::string_view name);
  Counter& operator+=(double value);
  Counter& operator=(double value);
  void operator()(double value);

  std::string_view name() const;

 private:
  struct Impl : public ITelemetryObject {
    explicit Impl(std::string_view name);

    void yield(double value);
    std::string_view name() const { return name_; }
    void capture(Sink& sink, const std::vector<Tag>& tags) override;

    std::string name_;
    mutable Sampler<double> sampler_;
  };

  std::shared_ptr<Impl> impl_;
};

}  // namespace bits::ttl
