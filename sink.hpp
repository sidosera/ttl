#pragma once

#include "types.hpp"

namespace bits::ttl {

class Sink {
 public:
  virtual ~Sink() = default;
  virtual void publish(Event&& event) = 0;
};

}  // namespace bits::ttl
