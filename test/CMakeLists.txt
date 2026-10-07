include(GoogleTest)

add_executable(cactus_board_tests
  board_test.cpp
  column_test.cpp
  store_test.cpp
  tools_test.cpp
)
target_link_libraries(cactus_board_tests PRIVATE cactus_board_core GTest::gtest_main)
gtest_discover_tests(cactus_board_tests)
