#include "measure.hpp"
#include "factory.hpp"
#include "file_sink.hpp"
#include <ring_buffer.hpp>
#include <gtest/gtest.h>
#include <cstdlib>
#include <thread>
#include <fstream>
#include <unordered_map>

using namespace bits::ttl;

class TestSink : public Sink {
public:
  void publish(const Event& event) override {
    last_name = event.name;
    last_timestamp = event.timestamp;
    last_fields.clear();
    for (size_t i = 0; i < event.field_count; ++i) {
      last_fields[std::string(event.fields[i].key)] = event.fields[i].value;
    }
    if (event.histogram) {
      last_hist = *event.histogram;
      hist_counts.assign(event.histogram->counts,
                         event.histogram->counts + event.histogram->bins);
      last_hist.counts = hist_counts.data();
    } else {
      last_hist.scale = HistScale::None;
    }
  }

  std::string last_name;
  int64_t last_timestamp{0};
  std::unordered_map<std::string, Value> last_fields;
  Histogram last_hist;
  std::vector<uint64_t> hist_counts;
};

TEST(MeasureTest, BasicAdd) {
  TestSink sink;
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};

  Measure m("test.metric", AVG);
  m += 10.0;
  m += 20.0;
  m += 30.0;

  m.grab(config);

  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("mean")), 20.0);
  EXPECT_EQ(std::get<int64_t>(sink.last_fields.at("count")), 3);
}

TEST(MeasureTest, AllAggregations) {
  TestSink sink;
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};

  Measure m("test.all", AVG | MIN | MAX | SUM);

  for (int i = 1; i <= 16; i++) {
    m += static_cast<double>(i);
  }

  m.grab(config);

  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("mean")), 8.5);
  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("min")), 1.0);
  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("max")), 16.0);
  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("sum")), 136.0);
  EXPECT_EQ(std::get<int64_t>(sink.last_fields.at("count")), 16);
  EXPECT_EQ(sink.last_hist.scale, HistScale::Log);
  EXPECT_EQ(sink.last_hist.bins, 128);
}

TEST(MeasureTest, OperatorAssignment) {
  TestSink sink;
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};

  Measure m("test.assign", SUM);
  m = 42.0;

  m.grab(config);

  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("sum")), 42.0);
}

TEST(MeasureTest, OperatorCall) {
  TestSink sink;
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};

  Measure m("test.call", SUM);
  m(42.0);

  m.grab(config);

  EXPECT_DOUBLE_EQ(std::get<double>(sink.last_fields.at("sum")), 42.0);
}

TEST(FactoryTest, CreateMeasure) {
  FileSink sink("/tmp/ttl_test_factory.log");
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};
  TtlFactory factory(config);

  auto& m1 = factory.measure("test.counter", SUM);
  auto& m2 = factory.measure("test.counter", AVG);

  EXPECT_EQ(&m1, &m2);
  EXPECT_EQ(m1.name(), "test.counter");

  unlink("/tmp/ttl_test_factory.log");
}

TEST(FactoryTest, MultipleMeasures) {
  FileSink sink("/tmp/ttl_test_multiple.log");
  Tag tags[] = {{"host", "test"}};
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 1, .flush_interval_seconds = 1};
  config.tags[0] = tags[0];
  TtlFactory factory(config);

  auto& writeLatency = factory.measure("socket.write_latency", AVG | MIN | MAX);
  auto& tickCount = factory.measure("socket.tick.count", SUM);
  auto& cpuLoad = factory.measure("cpu.load", AVG);

  writeLatency += 100.5;
  tickCount += 1;
  cpuLoad += 0.75;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  writeLatency += 200.3;
  tickCount += 1;
  cpuLoad += 0.80;

  unlink("/tmp/ttl_test_multiple.log");
}

TEST(FactoryTest, FlushLoop) {
  FileSink sink("/tmp/ttl_test_flush.log");
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};
  TtlFactory factory(config);

  auto& m = factory.measure("test.flush", SUM);

  for (int i = 0; i < 10; i++) {
    m += 1.0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  std::this_thread::sleep_for(std::chrono::seconds(2));

  unlink("/tmp/ttl_test_flush.log");
}

TEST(RingBufferTest, BasicOperations) {
  bits::RingBuffer<double, 4> rb;

  EXPECT_TRUE(rb.empty());
  EXPECT_FALSE(rb.full());
  EXPECT_EQ(rb.size(), 0);

  rb.push_back(1.0);
  EXPECT_FALSE(rb.empty());
  EXPECT_EQ(rb.size(), 1);
  EXPECT_DOUBLE_EQ(rb.front(), 1.0);

  rb.push_back(2.0);
  rb.push_back(3.0);
  rb.push_back(4.0);

  EXPECT_TRUE(rb.full());
  EXPECT_EQ(rb.size(), 4);

  rb.pop_front();
  EXPECT_EQ(rb.size(), 3);
  EXPECT_DOUBLE_EQ(rb.front(), 2.0);

  rb.clear();
  EXPECT_TRUE(rb.empty());
  EXPECT_EQ(rb.size(), 0);
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

  Tag tags[] = {{"host", "localhost"}};
  Field fields[] = {
      {"count", int64_t(10)},
      {"avg", 42.5}
  };
  uint64_t counts[4] = {1, 2, 3, 4};
  Histogram hist{
      .scale = HistScale::Log,
      .bins = 4,
      .start = 1e-6,
      .step = 0.0,
      .factor = 1.2,
      .underflow = 0,
      .counts = counts,
      .overflow = 0
  };

  Event event{
      .name = "test.metric",
      .timestamp = 1234567890,
      .tags = tags,
      .tag_count = 1,
      .fields = fields,
      .field_count = 2,
      .histogram = &hist
  };

  sink.publish(event);

  unlink("/tmp/ttl_test_sink.log");
}

TEST(HistogramTest, LogScaleDistribution) {
  TestSink sink;
  TtlConfig config{.sink = sink, .tags = {}, .tag_count = 0, .flush_interval_seconds = 1};

  Measure m("test.histogram", AVG);

  for (int i = 0; i < 100; ++i) {
    m += 0.001 * (i + 1);
  }

  m.grab(config);

  EXPECT_EQ(sink.last_hist.scale, HistScale::Log);
  EXPECT_EQ(sink.last_hist.bins, 128);
  EXPECT_DOUBLE_EQ(sink.last_hist.start, 1e-6);
  EXPECT_DOUBLE_EQ(sink.last_hist.factor, 1.2);

  uint64_t total_count = sink.last_hist.underflow + sink.last_hist.overflow;
  for (size_t i = 0; i < sink.last_hist.bins; ++i) {
    total_count += sink.hist_counts[i];
  }
  EXPECT_EQ(total_count, 100);
}
