#pragma once

#include "sink.hpp"

namespace bits::ttl {

class FileSink : public Sink {
 public:
  explicit FileSink(std::string_view path);
  ~FileSink();

  FileSink(const FileSink&) = delete;
  FileSink& operator=(const FileSink&) = delete;

  void publish(const Event& event) override;

 private:
  int fd_;
};

}  // namespace bits::ttl
