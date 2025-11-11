#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace bits::ttl {

using Value = std::variant<int64_t, double, std::string>;

struct Field {
  std::string key;
  Value value;
};

struct Event {
  std::string type;
  std::string name;
  std::chrono::nanoseconds timestamp;
  std::vector<Field> fields;
};

}  // namespace bits::ttl
