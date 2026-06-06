# tests/CMakeLists.txt

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE mystl GTest::gtest_main)

# Register test to CTest
include(GoogleTest)
gtest_discover_tests(test_utility)


add_executable(test_allocator test_allocator.cpp)
target_link_libraries(test_allocator PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_allocator)