#include "logger.hpp"
#include <chrono>
#include "runtime.hpp"
#include "sink.hpp"
#include "types.hpp"

using std::chrono::steady_clock;

namespace bits::ttl {

namespace detail {
LoggerImpl::LoggerImpl(std::string name) : name_(std::move(name)) {}

void LoggerImpl::yield(LogLevel level, const std::string& message) {
  Event event{
      .type      = "log",
      .name      = name_,
      .timestamp = steady_clock::now().time_since_epoch(),
      .fields    = {Field{.key = "level", .value = logLevelToString(level)},
                    Field{.key = "message", .value = message}}};

  q_.push(std::move(event));
}

void LoggerImpl::capture(ISink& sink) {
  while (auto event = q_.tryTake()) {
    sink.publish(std::move(*event));
  }
}
}  // namespace detail

Logger& Logger::instance() {
  static auto impl =
      detail::Runtime::instance()->makeObject<detail::LoggerImpl>("logger");
  static Logger logger{impl};
  return logger;
}

Logger::Logger() : Logger(detail::Runtime::instance()) {};

Logger::Logger(const std::shared_ptr<detail::Runtime>& runtime)
    : impl_(runtime->makeObject<detail::LoggerImpl>("logger")) {}

void Logger::log(LogLevel level, const std::string& message) {
  impl_->yield(level, message);
}

}  // namespace bits::ttl
