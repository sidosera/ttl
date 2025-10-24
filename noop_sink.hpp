#pragma once

#include "sink.hpp"

namespace bits::ttl {

class NoOp : public Sink {
 public:
  NoOp() = default;
  void publish(Event&& event) override;
};

}  // namespace bits::ttl
