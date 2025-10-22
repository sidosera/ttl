#pragma once

#include <cstdint>

namespace bits::ttl {

struct TtlConfig;

enum class Agg : uint32_t {
  COUNT = 1 << 0,
  P99 = 1 << 1,
  P50 = 1 << 2,
  AVG = 1 << 3,
  MIN = 1 << 4,
  MAX = 1 << 5,
  SUM = 1 << 6,
};

constexpr Agg operator|(Agg a, Agg b) {
  return static_cast<Agg>(static_cast<uint32_t>(a) |
                                      static_cast<uint32_t>(b));
}

constexpr bool operator&(Agg flags, Agg check) {
  return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(check)) != 0;
}

inline constexpr Agg COUNT = Agg::COUNT;
inline constexpr Agg P99 = Agg::P99;
inline constexpr Agg P50 = Agg::P50;
inline constexpr Agg AVG = Agg::AVG;
inline constexpr Agg MIN = Agg::MIN;
inline constexpr Agg MAX = Agg::MAX;
inline constexpr Agg SUM = Agg::SUM;

}  // namespace bits::ttl
