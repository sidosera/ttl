#pragma once

#include "sink.hpp"

namespace bits::ttl {

class Base : public ISink {
 public:
  explicit Base(int fd);
  ~Base() override;

  void publish(Event&& event) override;

  [[nodiscard]] int fd() const;

 private:
  int fd_{-1};
};

class File : public ISink {
 public:
  explicit File(std::string_view path);
  ~File() override;

  File(const File&)            = delete;
  File& operator=(const File&) = delete;

  void publish(Event&& event) override;

 private:
  Base sink_;
};

class Discard : public ISink {
 public:
  void publish(Event&& /*event*/) override {};
};

class StdOut : public ISink {
 public:
  explicit StdOut() : sink_(1) {};

  void publish(Event&& event) override { sink_.publish(std ::move(event)); }

 private:
  Base sink_;
};
}  // namespace bits::ttl
