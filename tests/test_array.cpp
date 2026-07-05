#include "mystl/array.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

using mystl::array;

TEST(ArrayTest, AggregateInitialization)
{
    array<int, 3> arr = {1, 2, 3};
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
}

TEST(ArrayTest, ElementAccess)
{
    array<int, 4> arr = {10, 20, 30, 40};
    
    EXPECT_EQ(arr.at(1), 20);
    EXPECT_EQ(arr.front(), 10);
    EXPECT_EQ(arr.back(), 40);
    
    // Проверка генерации исключения std::out_of_range
    EXPECT_THROW(arr.at(4), std::out_of_range);
    
    // Модификация через data()
    int* ptr = arr.data();
    ptr[0] = 99;
    EXPECT_EQ(arr.front(), 99);
}

TEST(ArrayTest, Iterators)
{
    array<int, 3> arr = {1, 2, 3};
    int sum = 0;
    
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);
    
    // Range-based for loop (работает благодаря begin/end)
    sum = 0;
    for (const auto& val : arr) {
        sum += val;
    }
    EXPECT_EQ(sum, 6);
}

TEST(ArrayTest, Capacity)
{
    array<double, 5> arr{};
    EXPECT_FALSE(arr.empty());
    EXPECT_EQ(arr.size(), 5);
    EXPECT_EQ(arr.max_size(), 5);
    
    array<int, 0> empty_arr{};
    EXPECT_TRUE(empty_arr.empty());
    EXPECT_EQ(empty_arr.size(), 0);
}

TEST(ArrayTest, Operations)
{
    array<int, 3> arr1;
    arr1.fill(7);
    EXPECT_EQ(arr1[0], 7);
    EXPECT_EQ(arr1[1], 7);
    EXPECT_EQ(arr1[2], 7);
    
    array<int, 3> arr2 = {1, 2, 3};
    arr1.swap(arr2);
    
    EXPECT_EQ(arr1[0], 1);
    EXPECT_EQ(arr2[0], 7);
}

TEST(ArrayTest, Comparisons)
{
    array<int, 3> a = {1, 2, 3};
    array<int, 3> b = {1, 2, 3};
    array<int, 3> c = {1, 2, 4};
    
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
}

TEST(ArrayTest, StructuredBindings)
{
    array<int, 3> arr = {100, 200, 300};
    
    auto [x, y, z] = arr;
    EXPECT_EQ(x, 100);
    EXPECT_EQ(y, 200);
    EXPECT_EQ(z, 300);
    
    // Проверка ссылок
    auto& [rx, ry, rz] = arr;
    rx = 999;
    EXPECT_EQ(arr[0], 999);
}