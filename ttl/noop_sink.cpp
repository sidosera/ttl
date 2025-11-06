#include "noop_sink.hpp"
#include <unistd.h>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "noop_sink.hpp"

namespace bits::ttl {

void NoOp::publish(Event&& event) {
  static volatile Event _ = event;
}

}  // namespace bits::ttl
