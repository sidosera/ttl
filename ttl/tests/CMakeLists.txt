add_executable(
  counter_test
  counter_test.cpp
)

target_link_libraries(
  counter_test
  PRIVATE
  ttl
  GTest::gtest_main
)

add_executable(
  bench
  bench.cpp
)

target_link_libraries(
  bench
  PRIVATE
  ttl
  bits
  benchmark::benchmark
)

add_executable(
  logger_test
  logger_test.cpp
)

target_link_libraries(
  logger_test
  PRIVATE
  ttl
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(counter_test)
gtest_discover_tests(logger_test)
