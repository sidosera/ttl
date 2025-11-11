include(GoogleTest)

add_executable(algo_test algo_test.cpp)

target_link_libraries(algo_test PRIVATE bits GTest::gtest_main)

target_include_directories(algo_test PRIVATE ${CMAKE_SOURCE_DIR})

gtest_discover_tests(algo_test)

add_test(
  NAME algo_test
  COMMAND $<TARGET_FILE:algo_test>
)
