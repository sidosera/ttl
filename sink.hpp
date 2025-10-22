#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <variant>

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

enum class HistScale : uint8_t {
  None,
  Linear,
  Log
};

struct Histogram {
  HistScale scale{HistScale::None};
  uint16_t bins{0};
  double start{0.0};
  double step{0.0};
  double factor{0.0};
  uint64_t underflow{0};
  const uint64_t* counts{nullptr};
  uint64_t overflow{0};
};

struct Event {
  std::string_view name;
  int64_t timestamp;
  const Tag* tags;
  size_t tag_count;
  const Field* fields;
  size_t field_count;
  const Histogram* histogram{nullptr};
};

class Sink {
public:
  virtual ~Sink() = default;
  virtual void publish(const Event& event) = 0;
};

}
