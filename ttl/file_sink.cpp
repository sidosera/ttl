#include "file_sink.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "throw_if_not.hpp"
#include "types.hpp"

namespace bits::ttl {

Base::Base(int fd) : fd_(fd) {};

Base::~Base() {
  if (fd_ >= 0) {
    ::close(fd_);
  }
}

[[nodiscard]] int Base::fd() const {
  return fd_;
}

void Base::publish(Event&& event) {
  if (fd_ < 0) {
    return;
  }

  char buf[4096];
  char* p   = buf;
  char* end = buf + sizeof(buf) - 1;

  auto append = [&](const char* s) {
    while (*s && p < end) {
      *p++ = *s++;
    }
  };

  auto append_sv = [&](std::string_view sv) {
    size_t n = std::min(static_cast<size_t>(end - p), sv.size());
    std::memcpy(p, sv.data(), n);
    p += n;
  };

  auto append_int = [&](int64_t val) {
    auto [ptr, ec] = std::to_chars(p, end, val);
    if (ec == std::errc()) {
      p = ptr;
    }
  };

  auto append_double = [&](double val) {
    auto [ptr, ec] = std::to_chars(p, end, val);
    if (ec == std::errc()) {
      p = ptr;
    }
  };

  append(R"({"type":")");
  append_sv(event.type);
  append(R"(","name":")");
  append_sv(event.name);
  append(R"(","ts":)");
  append_int(event.timestamp.count());

  for (auto& field : event.fields) {
    append(",\"");
    append_sv(field.key);
    append("\":");
    std::visit(
        [&](auto&& v) {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, int64_t>) {
            append_int(v);
          } else if constexpr (std::is_same_v<T, double>) {
            append_double(v);
          } else if constexpr (std::is_same_v<T, std::string_view>) {
            append("\"");
            append_sv(v);
            append("\"");
          }
        },
        field.value);
  }

  append("}\n");

  size_t len = p - buf;
  ::write(fd_, buf, len);
}

File::File(std::string_view path)
    : sink_(open(std::string(path).c_str(), O_WRONLY | O_APPEND | O_CREAT,
                 0644)) {
  const int err = errno;
  bits::throwIfNot(sink_.fd() >= 0, "ttl: failed to open {}: {}", path,
                   std::error_code(err, std::generic_category()).message());
}

File::~File() = default;

void File::publish(Event&& event) {
  sink_.publish(std::move(event));
}

}  // namespace bits::ttl
