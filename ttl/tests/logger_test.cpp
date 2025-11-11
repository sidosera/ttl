#include "logger.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <vector>
#include "runtime.hpp"
#include "types.hpp"

using namespace bits::ttl;

class MockSink : public ISink {
 public:
  explicit MockSink(std::shared_ptr<std::vector<Event>> events)
      : events_(std::move(events)) {};

  void publish(Event&& event) override {
    std::unique_lock lock(mutex_);
    events_->push_back(std::move(event));
  }

  [[nodiscard]] std::vector<Event> events() const {
    std::vector<Event> copy;
    {
      std::unique_lock lock(mutex_);
      copy.insert(copy.begin(), events_->begin(), events_->end());
    }

    return copy;
  }

 private:
  mutable std::mutex mutex_;
  std::shared_ptr<std::vector<Event>> events_;
};

TEST(LoggerTest, BasicLogging) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));
    Logger logger{rt};
    LogStream(logger, Info) << "test message";
  }

  ASSERT_GE(events->size(), 1);

  bool found = false;
  for (const auto& event : *events) {
    if (event.name == "logger") {
      found = true;
      EXPECT_EQ(event.type, "log");
      ASSERT_GE(event.fields.size(), 2);

      bool has_level   = false;
      bool has_message = false;

      for (const auto& field : event.fields) {
        if (field.key == "level") {
          has_level = true;
          EXPECT_EQ(std::get<std::string>(field.value), "Info");
        }
        if (field.key == "message") {
          has_message = true;
          EXPECT_EQ(std::get<std::string>(field.value), "test message");
        }
      }

      EXPECT_TRUE(has_level);
      EXPECT_TRUE(has_message);
    }
  }

  EXPECT_TRUE(found);
}

TEST(LoggerTest, MultipleLogLevels) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));
    Logger logger{rt};
    LogStream(logger, Trace) << "trace";
    LogStream(logger, Debug) << "debug";
    LogStream(logger, Info) << "info";
    LogStream(logger, Warn) << "warn";
    LogStream(logger, Error) << "error";
    LogStream(logger, Critical) << "critical";
  }

  int log_count = 0;
  for (const auto& event : *events) {
    if (event.type == "log") {
      log_count++;
    }
  }

  EXPECT_EQ(log_count, 6);
}

TEST(LoggerTest, StreamOperator) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));
    Logger logger{rt};
    int value = 42;
    LogStream(logger, Info) << "value=" << value;
  }

  bool found = false;
  for (const auto& event : *events) {
    if (event.type == "log") {
      for (const auto& field : event.fields) {
        if (field.key == "message") {
          const auto& msg = std::get<std::string>(field.value);
          if (msg.find("value=42") != std::string::npos) {
            found = true;
          }
        }
      }
    }
  }

  EXPECT_TRUE(found);
}
