#pragma once

#include "types.hpp"

namespace bits::ttl {

class ISink {
 public:
  virtual ~ISink() = default;
  virtual void publish(Event&& event) = 0;
};

}  // namespace bits::ttl
