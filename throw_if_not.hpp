#pragma once

#include <format>
#include <functional>
#include <stdexcept>

namespace bits {
template <std::invocable Pred, class... Args>
inline void throwIfNot(Pred&& pred, std::format_string<Args...> fmt,
                         Args&&... args) {
  if (!std::invoke(std::forward<Pred>(pred))) {
    throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
  }
}

template <class... Args>
inline void throwIfNot(bool ok, std::format_string<Args...> fmt,
                         Args&&... args) {
  if (!ok) {
    throw std::runtime_error(std::format(fmt, std::forward<Args>(args)...));
  }
}

}  // namespace bits
