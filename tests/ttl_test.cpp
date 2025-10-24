#include "ttl.hpp"
#include <gtest/gtest.h>
#include <cstdlib>
#include <ring_buffer.hpp>
#include <thread>
#include "counter.hpp"
#include "file_sink.hpp"

using namespace bits::ttl;

TEST(CounterTest, BasicAdd) {
  Ttl::init("test://");

  Counter c("test.metric");
  c += 10.0;
  c += 20.0;
  c += 30.0;

  Ttl::shutdown();

  auto& events = TestSink::events();

  ASSERT_GE(events.size(), 1);

  for (const auto& event : events) {
    EXPECT_EQ(event.name, "test.metric");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (std::string_view(field.key) == "value") {
        has_value = true;
        double val = std::get<double>(field.value);
        EXPECT_TRUE(val == 10.0 || val == 20.0 || val == 30.0);
      }
    }
    EXPECT_TRUE(has_value);
  }

  TestSink::reset();
}

TEST(CounterTest, AllAggregations) {
  TestSink::reset();
  Ttl::init("test://");

  Counter c("test.all");
  for (int i = 1; i <= 16; i++) {
    c += static_cast<double>(i);
  }

  Ttl::shutdown();

  auto& events = TestSink::events();
  ASSERT_GE(events.size(), 1);

  for (const auto& event : events) {
    EXPECT_EQ(event.name, "test.all");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (std::string_view(field.key) == "value") {
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
  TestSink::reset();
  Ttl::init("test://");

  Counter c("test.assign");
  c = 42.0;

  Ttl::shutdown();

  auto& events = TestSink::events();
  ASSERT_GE(events.size(), 1);

  for (const auto& event : events) {
    EXPECT_EQ(event.name, "test.assign");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (std::string_view(field.key) == "value") {
        has_value = true;
        EXPECT_DOUBLE_EQ(std::get<double>(field.value), 42.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(CounterTest, OperatorCall) {
  TestSink::reset();
  Ttl::init("test://");

  Counter c("test.call");
  c(42.0);

  Ttl::shutdown();

  auto& events = TestSink::events();
  ASSERT_GE(events.size(), 1);

  for (const auto& event : events) {
    EXPECT_EQ(event.name, "test.call");

    bool has_value = false;
    for (const auto& field : event.fields) {
      if (std::string_view(field.key) == "value") {
        has_value = true;
        EXPECT_DOUBLE_EQ(std::get<double>(field.value), 42.0);
      }
    }
    EXPECT_TRUE(has_value);
  }
}

TEST(TtlTest, InitAndShutdown) {
  Ttl::init("file:///tmp/ttl_test_init.log");
  Counter c("test.counter");
  c += 1.0;
  Ttl::shutdown();

  unlink("/tmp/ttl_test_init.log");
}

TEST(TtlTest, SharedImplementation) {
  Ttl::init("noop://");

  Counter c1("test.shared");
  Counter c2("test.shared");

  EXPECT_EQ(c1.name(), c2.name());
  EXPECT_EQ(c1.name(), "test.shared");

  Ttl::shutdown();
}

TEST(TtlTest, MultipleCounters) {
  TestSink::reset();
  Ttl::init("test://");

  Counter writeLatency("socket.write_latency");
  Counter tickCount("socket.tick.count");
  Counter cpuLoad("cpu.load");

  writeLatency += 100.5;
  tickCount += 1;
  cpuLoad += 0.75;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  writeLatency += 200.3;
  tickCount += 1;
  cpuLoad += 0.80;

  Ttl::shutdown();

  auto& events = TestSink::events();
  EXPECT_GE(events.size(), 3);

  int write_latency_count = 0;
  int tick_count_count = 0;
  int cpu_load_count = 0;

  for (const auto& event : events) {
    if (event.name == "socket.write_latency") write_latency_count++;
    if (event.name == "socket.tick.count") tick_count_count++;
    if (event.name == "cpu.load") cpu_load_count++;
  }

  EXPECT_GE(write_latency_count, 1);
  EXPECT_GE(tick_count_count, 1);
  EXPECT_GE(cpu_load_count, 1);
}

TEST(TtlTest, FlushLoop) {
  Ttl::init("file:///tmp/ttl_test_flush.log");

  Counter c("test.flush");

  for (int i = 0; i < 10; i++) {
    c += 1.0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));

  Ttl::shutdown();
  unlink("/tmp/ttl_test_flush.log");
}

TEST(TtlTest, IdempotentInit) {
  Ttl::init("noop://");
  Ttl::init("noop://");

  Counter c("test.idempotent");
  c += 1.0;

  Ttl::shutdown();
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
  FileSink sink("/tmp/ttl_test_sink.log");

  Event event{
      .name = "test.metric",
      .timestamp = 1234567890,
      .tags = {{"host", "localhost"}},
      .fields = {{"count", int64_t(10)}, {"avg", 42.5}},
  };

  sink.publish(std::move(event));

  unlink("/tmp/ttl_test_sink.log");
}
