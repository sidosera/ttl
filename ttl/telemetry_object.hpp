#pragma once

#include <memory>
#include <vector>

namespace bits::ttl {

class Sink;
struct Tag;

struct ITelemetryObject {
  virtual ~ITelemetryObject() = default;
  virtual void capture(Sink& sink, const std::vector<Tag>& tags) = 0;
};

using ITelemetryObjectPtr = std::shared_ptr<ITelemetryObject>;

}  // namespace bits::ttl
