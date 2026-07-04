# tests/CMakeLists.txt

include(GoogleTest)

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_utility)

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

add_executable(test_queue test_queue.cpp)
target_link_libraries(test_queue PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_queue)

add_executable(test_priority_queue test_priority_queue.cpp)
target_link_libraries(test_priority_queue PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_priority_queue)

add_executable(test_unordered_map test_unordered_map.cpp)
target_link_libraries(test_unordered_map PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_unordered_map)

add_executable(test_unordered_set test_unordered_set.cpp)
target_link_libraries(test_unordered_set PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_unordered_set)

add_executable(test_unordered_multiset test_unordered_multiset.cpp)
target_link_libraries(test_unordered_multiset PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_unordered_multiset)

add_executable(test_unordered_multimap test_unordered_multimap.cpp)
target_link_libraries(test_unordered_multimap PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_unordered_multimap)

add_executable(test_set test_set.cpp)
target_link_libraries(test_set PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_set)

add_executable(test_map test_map.cpp)
target_link_libraries(test_map PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_map)

add_executable(test_multiset test_multiset.cpp)
target_link_libraries(test_multiset PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_multiset)

add_executable(test_multimap test_multimap.cpp)
target_link_libraries(test_multimap PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_multimap)

add_executable(test_algorithm test_algorithm.cpp)
target_link_libraries(test_algorithm PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_algorithm)

add_executable(test_allocator test_allocator.cpp)
target_link_libraries(test_allocator PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_allocator)

add_executable(test_hash_table test_hash_table.cpp)
target_link_libraries(test_hash_table PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_hash_table)

add_executable(test_type_traits test_type_traits.cpp)
target_link_libraries(test_type_traits PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_type_traits)

add_executable(test_pair test_pair.cpp)
target_link_libraries(test_pair PRIVATE mystl GTest::gtest_main)
gtest_discover_tests(test_pair)