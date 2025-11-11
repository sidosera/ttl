#pragma once

#include <cstdint>
#include <memory>
#include <bits/queue.hpp>
#include <sstream>
#include <string>
#include "telemetry_object.hpp"
#include "types.hpp"

namespace bits::ttl {

enum class LogLevel : uint8_t {
  Trace    = 0,
  Debug    = 1,
  Info     = 2,
  Warn     = 3,
  Error    = 4,
  Critical = 5,
};

inline std::string logLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::Trace:
      return "Trace";
    case LogLevel::Debug:
      return "Debug";
    case LogLevel::Info:
      return "Info";
    case LogLevel::Warn:
      return "Warn";
    case LogLevel::Error:
      return "Error";
    case LogLevel::Critical:
      return "Critical";
  }
}

class ISink;

namespace detail {
class Runtime;

struct LoggerImpl : public ITelemetryObject {
  explicit LoggerImpl(std::string name);
  void yield(LogLevel level, const std::string& message);
  void capture(ISink& sink) override;
  bits::MPSCQueue<Event> q_;
  std::string name_;
};

}  // namespace detail

class Logger {
 public:
  static Logger& instance();
  explicit Logger();
  explicit Logger(const std::shared_ptr<detail::Runtime>& runtime);

  void log(LogLevel level, const std::string& message);

 private:
  explicit Logger(std::shared_ptr<detail::LoggerImpl> impl)
      : impl_(std::move(impl)) {}

  std::shared_ptr<detail::LoggerImpl> impl_;
};

class LogStream {
 public:
  LogStream(Logger& logger, LogLevel level) : logger_(logger), level_(level) {}

  ~LogStream() { logger_.log(level_, stream_.str()); }

  LogStream(const LogStream&)            = delete;
  LogStream& operator=(const LogStream&) = delete;
  LogStream(LogStream&&)                 = delete;
  LogStream& operator=(LogStream&&)      = delete;

  template <typename T>
  LogStream& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

 private:
  Logger& logger_;
  LogLevel level_;
  std::ostringstream stream_;
};

#ifndef TTL_LOG_MIN_LEVEL
#define TTL_LOG_MIN_LEVEL 0
#endif

#define TTL_LOG_ENABLED(level) (static_cast<int>(level) >= TTL_LOG_MIN_LEVEL)

#define TTL_LOG(level)                  \
  if constexpr (TTL_LOG_ENABLED(level)) \
  ::bits::ttl::LogStream(::bits::ttl::Logger::instance(), level)

constexpr auto Trace    = LogLevel::Trace;
constexpr auto Debug    = LogLevel::Debug;
constexpr auto Info     = LogLevel::Info;
constexpr auto Warn     = LogLevel::Warn;
constexpr auto Error    = LogLevel::Error;
constexpr auto Critical = LogLevel::Critical;

}  // namespace bits::ttl
