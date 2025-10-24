#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#include <thread>
#include "ttl.hpp"
#include "counter.hpp"

using namespace bits::ttl;

int main() {
	Ttl::init("file:///tmp/ttl_example.log");

  Counter latency_p99("demo.latency");
  Counter throughput("demo.throughput");
  Counter cpu_usage("demo.cpu.usage");
  Counter queue_depth("demo.queue.depth");
  Counter error_rate("demo.error.rate");

	std::cout << "Generating metrics to /tmp/ttl_example.log (Ctrl+C to stop)..."
						<< std::endl;

  constexpr double two_pi = 6.283185307179586;
  constexpr std::array<double, 12> latency_pattern = {
      85.0, 90.0, 95.0, 110.0, 130.0, 150.0, 110.0, 95.0, 90.0, 85.0, 80.0, 75.0};

  for (int i = 0;; ++i) {
    const double seconds = static_cast<double>(i) * 0.001;

    const double coarse_latency =
        latency_pattern[static_cast<size_t>(i) % latency_pattern.size()];
    const double fine_latency = 4.0 * std::sin(seconds * 1.5);

    latency_p99 += coarse_latency + fine_latency;

    const double throughput_base =
        500.0 + 80.0 * std::sin(seconds * 0.6) + (i % 20) * 5.0;
    throughput += throughput_base;

    const double cpu = 0.35 + 0.25 * std::sin(seconds * 0.3) + 0.05 * std::sin(seconds * 2.2);
    cpu_usage += cpu;

    const double depth = 20.0 + 15.0 * std::sin(seconds * 0.8 + two_pi / 3.0);
    queue_depth += depth;

    const double error = std::max(0.0, std::min(1.0, 0.01 + 0.008 * std::sin(seconds * 1.2 + two_pi / 4.0)));
    error_rate += error;

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

	Ttl::shutdown();
	return 0;
}
