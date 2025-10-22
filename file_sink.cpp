#include "file_sink.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <charconv>
#include <string>
#include <fcntl.h>
#include <unistd.h>

namespace bits::ttl {

FileSink::FileSink(std::string_view path) : fd_(-1) {
  std::string path_str(path);
  fd_ = open(path_str.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644);
  if (fd_ < 0) {
    fprintf(stderr, "ttl: failed to open %s: %s\n", path_str.c_str(), strerror(errno));
    abort();
  }
}

FileSink::~FileSink() {
  if (fd_ >= 0) {
    close(fd_);
  }
}

void FileSink::publish(const Event& event) {
  if (fd_ < 0) {
    return;
  }

  char buf[4096];
  char* p = buf;
  char* end = buf + sizeof(buf) - 1;

  auto append = [&](const char* s) {
    while (*s && p < end) *p++ = *s++;
  };

  auto append_sv = [&](std::string_view sv) {
    size_t n = std::min(static_cast<size_t>(end - p), sv.size());
    std::memcpy(p, sv.data(), n);
    p += n;
  };

  auto append_int = [&](int64_t val) {
    auto [ptr, ec] = std::to_chars(p, end, val);
    if (ec == std::errc()) p = ptr;
  };

  auto append_double = [&](double val) {
    auto [ptr, ec] = std::to_chars(p, end, val);
    if (ec == std::errc()) p = ptr;
  };

  append("{\"name\":\"");
  append_sv(event.name);
  append("\",\"ts\":");
  append_int(event.timestamp);

  for (size_t i = 0; i < event.tag_count; ++i) {
    append(",\"tag_");
    append_sv(event.tags[i].key);
    append("\":\"");
    append_sv(event.tags[i].value);
    append("\"");
  }

  for (size_t i = 0; i < event.field_count; ++i) {
    append(",\"");
    append_sv(event.fields[i].key);
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
        event.fields[i].value);
  }

  if (event.histogram && event.histogram->scale != HistScale::None) {
    const auto& h = *event.histogram;
    append(",\"scale\":\"");
    append(h.scale == HistScale::Linear ? "linear" : "log");
    append("\",\"bins\":");
    append_int(h.bins);
    append(",\"start\":");
    append_double(h.start);
    if (h.scale == HistScale::Linear) {
      append(",\"step\":");
      append_double(h.step);
    } else {
      append(",\"factor\":");
      append_double(h.factor);
    }
    append(",\"underflow\":");
    append_int(h.underflow);
    append(",\"counts\":[");
    for (uint16_t i = 0; i < h.bins; ++i) {
      if (i > 0) append(",");
      append_int(h.counts[i]);
    }
    append("],\"overflow\":");
    append_int(h.overflow);
  }

  append("}\n");

  size_t len = p - buf;
  ssize_t n = write(fd_, buf, len);
  if (n != static_cast<ssize_t>(len)) {
    fprintf(stderr, "ttl: write failed: %s (wrote %zd of %zu bytes)\n",
            strerror(errno), n > 0 ? n : 0, len);
    abort();
  }
}

}  // namespace bits::ttl
