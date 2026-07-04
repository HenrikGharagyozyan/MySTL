#include <gtest/gtest.h>
#include "mystl/utility.hpp"
#include "mystl/pair.hpp"
#include <string>

// Test Move semantics
TEST(UtilityTest, MoveSemantics)
{
    std::string original = "hello";
    std::string moved = mystl::move(original);

    EXPECT_EQ(moved, "hello");
    // After moving, the original string should be empty (std::string specific behavior)
    EXPECT_TRUE(original.empty());
}

// Test Pair creation
TEST(UtilityTest, PairCreation)
{
    mystl::Pair<int, double> p(1, 3.14);
    EXPECT_EQ(p.first, 1);
    EXPECT_DOUBLE_EQ(p.second, 3.14);
}

// Test make_pair and perfect forwarding
TEST(UtilityTest, MakePair)
{
    auto p = mystl::make_pair(42, std::string("test"));
    EXPECT_EQ(p.first, 42);
    EXPECT_EQ(p.second, "test");
}

// Test Swap
TEST(UtilityTest, PairSwap)
{
    auto p1 = mystl::make_pair(1, 2);
    auto p2 = mystl::make_pair(3, 4);
    p1.swap(p2);

    EXPECT_EQ(p1.first, 3);
    EXPECT_EQ(p1.second, 4);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
}