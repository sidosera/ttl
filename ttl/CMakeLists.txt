add_library(
  ttl
  file_sink.hpp
  file_sink.cpp
  sink.hpp
  types.hpp
  ttl.hpp
  ttl.cpp
  telemetry_object.hpp
  counter.hpp
  counter.cpp
  runtime.hpp
  runtime.cpp
  logger.hpp
  logger.cpp
)

target_link_libraries(
  ttl
  PUBLIC
  bits
)

target_include_directories(
  ttl
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)


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

add_subdirectory(tests)
