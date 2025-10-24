add_executable(
  ttl_test
  ttl_test.cpp
)

target_link_libraries(
  ttl_test
  PRIVATE
  ttl
  GTest::gtest_main
)

add_executable(
  sampler_test
  sampler_test.cpp
)

target_link_libraries(
  sampler_test
  PRIVATE
  ttl
  GTest::gtest_main
)

add_executable(
  metric_bench
  metric_bench.cpp
)

target_link_libraries(
  metric_bench
  PRIVATE
  ttl
  bits
  benchmark::benchmark
)

include(GoogleTest)
gtest_discover_tests(ttl_test)
gtest_discover_tests(sampler_test)
