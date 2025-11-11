add_library(bits INTERFACE)

target_include_directories(
  bits
  INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(bits INTERFACE simde)

add_subdirectory(ttl)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()