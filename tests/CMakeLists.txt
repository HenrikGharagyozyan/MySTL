# tests/CMakeLists.txt

include(GoogleTest)

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_utility)

add_executable(test_allocator test_allocator.cpp)
target_link_libraries(test_allocator PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_allocator)

add_executable(test_vector test_vector.cpp)
target_link_libraries(test_vector PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_vector)

add_executable(test_string test_string.cpp)
target_link_libraries(test_string PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_string)

add_executable(test_list test_list.cpp)
target_link_libraries(test_list PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_list)

add_executable(test_forward_list test_forward_list.cpp)
target_link_libraries(test_forward_list PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_forward_list)

add_executable(test_deque test_deque.cpp)
target_link_libraries(test_deque PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_deque)

add_executable(test_stack test_stack.cpp)
target_link_libraries(test_stack PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_stack)
