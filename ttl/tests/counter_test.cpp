#include <gtest/gtest.h>
#include <ring_buffer.hpp>
#include <thread>
#include <vector>
#include "counter.hpp"
#include "file_sink.hpp"
#include "runtime.hpp"

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


TEST(CounterTest, BasicAdd) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));

    Counter c("test.metric", rt);
    c += 10.0;
    c += 20.0;
    c += 30.0;
  }

  ASSERT_GE(events->size(), 1);

  for (const auto& event : *events) {
    EXPECT_EQ(event.name, "test.metric");
    EXPECT_EQ(event.type, "metric");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (field.key == "value") {
        has_value = true;
        double val = std::get<double>(field.value);
        EXPECT_TRUE(val == 10.0 || val == 20.0 || val == 30.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(CounterTest, AllAggregations) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));

    Counter c("test.all", rt);
    for (int i = 1; i <= 16; i++) {
      c += static_cast<double>(i);
    }
  }

  ASSERT_GE(events->size(), 1);

  for (const auto& event : *events) {
    EXPECT_EQ(event.name, "test.all");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (field.key == "value") {
        has_value = true;
        double val = std::get<double>(field.value);
        EXPECT_GE(val, 1.0);
        EXPECT_LE(val, 16.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(CounterTest, OperatorAssignment) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));

    Counter c("test.assign", rt);
    c = 42.0;
  }

  ASSERT_GE(events->size(), 1);

  for (const auto& event : *events) {
    EXPECT_EQ(event.name, "test.assign");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (field.key == "value") {
        has_value = true;
        EXPECT_DOUBLE_EQ(std::get<double>(field.value), 42.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(CounterTest, OperatorCall) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));

    Counter c("test.call", rt);
    c(42.0);
  }

  ASSERT_GE(events->size(), 1);

  for (const auto& event : *events) {
    EXPECT_EQ(event.name, "test.call");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (field.key == "value") {
        has_value = true;
        EXPECT_DOUBLE_EQ(std::get<double>(field.value), 42.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(CounterTest, InitAndShutdown) {
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<File>("/tmp/ttl_test_init.log"));

    Counter c("test.counter", rt);
    c += 1.0;
  }

  unlink("/tmp/ttl_test_init.log");
}

TEST(CounterTest, SharedImplementation) {
  auto rt = std::make_shared<detail::Runtime>();
  rt->init(std::make_unique<Discard>());

  Counter c1("test.shared", rt);
  Counter c2("test.shared", rt);

  EXPECT_EQ(c1.name(), c2.name());
  EXPECT_EQ(c1.name(), "test.shared");
}

TEST(CounterTest, MultipleCounters) {
  auto events = std::make_shared<std::vector<Event>>();
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<MockSink>(events));

    Counter writeLatency("socket.write_latency", rt);
    Counter tickCount("socket.tick.count", rt);
    Counter cpuLoad("cpu.load", rt);

    writeLatency += 100.5;
    tickCount += 1;
    cpuLoad += 0.75;

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    writeLatency += 200.3;
    tickCount += 1;
    cpuLoad += 0.80;
  }

  EXPECT_GE(events->size(), 3);

  int write_latency_count = 0;
  int tick_count_count = 0;
  int cpu_load_count = 0;

  for (const auto& event : *events) {
    if (event.name == "socket.write_latency") {
      write_latency_count++;
    }
    if (event.name == "socket.tick.count") {
      tick_count_count++;
    }
    if (event.name == "cpu.load") {
      cpu_load_count++;
    }
  }

  EXPECT_GE(write_latency_count, 1);
  EXPECT_GE(tick_count_count, 1);
  EXPECT_GE(cpu_load_count, 1);
}

TEST(CounterTest, FlushLoop) {
  {
    auto rt = std::make_shared<detail::Runtime>();
    rt->init(std::make_unique<File>("/tmp/ttl_test_flush.log"));

    Counter c("test.flush", rt);

    for (int i = 0; i < 10; i++) {
      c += 1.0;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  unlink("/tmp/ttl_test_flush.log");
}

TEST(CounterTest, IdempotentInit) {
  auto rt = std::make_shared<detail::Runtime>();
  rt->init(std::make_unique<Discard>());

  // Second init should throw
  EXPECT_THROW(rt->init(std::make_unique<Discard>()), std::runtime_error);

  Counter c("test.idempotent", rt);
  c += 1.0;
}

TEST(RingBufferTest, Wraparound) {
  bits::RingBuffer<double, 4> rb;

  for (int i = 0; i < 8; i++) {
    rb.push_back(static_cast<double>(i));
  }

  EXPECT_EQ(rb.size(), 4);
  EXPECT_DOUBLE_EQ(rb[0], 4.0);
  EXPECT_DOUBLE_EQ(rb[1], 5.0);
  EXPECT_DOUBLE_EQ(rb[2], 6.0);
  EXPECT_DOUBLE_EQ(rb[3], 7.0);
}

TEST(FileSinkTest, WriteEvents) {
  File sink("/tmp/ttl_test_sink.log");

  Event event{
      .type = "metric",
      .name = "test.metric",
      .timestamp = std::chrono::nanoseconds(1234567890),
      .fields = {{"count", int64_t(10)}, {"avg", 42.5}},
  };

  sink.publish(std::move(event));

  unlink("/tmp/ttl_test_sink.log");
}
