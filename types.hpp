#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace bits::ttl {

using Value = std::variant<int64_t, double, std::string_view>;

struct Tag {
  std::string_view key;
  std::string_view value;
};

struct Field {
  std::string_view key;
  Value value;
};

struct Event {
  std::string name;
  int64_t timestamp;
  std::vector<Tag> tags;
  std::vector<Field> fields;
};

}  // namespace bits::ttl
