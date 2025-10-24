#pragma once

#include <string_view>

namespace bits::ttl {

class Ttl {
 public:
  Ttl() = delete;
  Ttl(const Ttl&) = delete;
  Ttl(Ttl&&) = delete;
  Ttl& operator=(const Ttl&) = delete;
  Ttl& operator=(Ttl&&) = delete;

  static void init(std::string_view connectionString);
  static void shutdown();
};

}  // namespace bits::ttl
