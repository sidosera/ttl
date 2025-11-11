#pragma once

#include <memory>

namespace bits::ttl {

class ISink;

struct ITelemetryObject {
  virtual ~ITelemetryObject()       = default;
  virtual void capture(ISink& sink) = 0;
};

using ITelemetryObjectPtr = std::shared_ptr<ITelemetryObject>;

}  // namespace bits::ttl
