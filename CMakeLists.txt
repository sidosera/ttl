add_library(
  ttl
  measure.hpp
  measure.cpp
  factory.hpp
  factory.cpp
  file_sink.hpp
  file_sink.cpp
  sink.hpp
  types.hpp
)

target_link_libraries(
  ttl
  PRIVATE
  fmt::fmt
  PUBLIC
  bits
)

target_include_directories(
  ttl
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

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

include(GoogleTest)
gtest_discover_tests(ttl_test)

add_executable(
  example
  example.cpp
)

target_link_libraries(
  example
  PRIVATE
  ttl
  bits
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