#pragma once

#include <functional>

namespace bits {
template <class T>
struct Hash {
  size_t operator()(const T& v) const noexcept(noexcept(std::hash<T>{}(v))) {
    return std::hash<T>{}(v);
  }
};

template <class T1, class T2>
struct Hash<std::pair<T1, T2>> {
  size_t operator()(const std::pair<T1, T2>& p) const
      noexcept(noexcept(Hash<T1>{}(p.first)) &&
               noexcept(Hash<T2>{}(p.second))) {
    auto h1 = Hash<T1>{}(p.first);
    auto h2 = Hash<T2>{}(p.second);
    return h1 ^ (h2 << 1);
  }
};
}  // namespace bits