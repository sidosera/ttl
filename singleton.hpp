#pragma once

#include <memory>

namespace bits {

template <class T>
struct Ctor {
  T* operator()() const { return new T{}; }
};

template <typename T, auto Factory = Ctor<T>()>
class Singleton {
 public:
  Singleton(Singleton const&)            = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton(Singleton&&)                 = default;
  Singleton& operator=(Singleton&&)      = default;

  static std::shared_ptr<T> instance() {
    static auto t = std::shared_ptr<T>(Factory());
    return t;
  }

 protected:
  friend struct Ctor<T>;
  Singleton() = default;

 private:
  static T* p;
};
}  // namespace bits
