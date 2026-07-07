#include <gtest/gtest.h>
#include "mystl/pair.hpp"
#include <string>
#include <type_traits>

using namespace mystl;

// ============================================================================
// 1. Constructors
// ============================================================================

TEST(PairTest, DefaultConstructor)
{
    Pair<int, double> p;
    EXPECT_EQ(p.first, 0);
    EXPECT_EQ(p.second, 0.0);
}

TEST(PairTest, ValueConstructor)
{
    Pair<int, std::string> p(42, "hello");
    EXPECT_EQ(p.first, 42);
    EXPECT_EQ(p.second, "hello");
}

TEST(PairTest, CopyAndMoveConstructor)
{
    Pair<int, int> p1(1, 2);
    
    // Copying
    Pair<int, int> p2(p1);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);

    // Moving
    Pair<int, int> p3(mystl::move(p1));
    EXPECT_EQ(p3.first, 1);
    EXPECT_EQ(p3.second, 2);
}

// ============================================================================
// 2. make_pair and perfect forwarding (decay_t)
// ============================================================================

TEST(PairTest, MakePair)
{
    auto p = mystl::make_pair(10, 3.14);
    
    // Check types to ensure decay_t works correctly
    static_assert(std::is_same_v<decltype(p.first), int>, "first should be int");
    static_assert(std::is_same_v<decltype(p.second), double>, "second should be double");
    
    EXPECT_EQ(p.first, 10);
    EXPECT_EQ(p.second, 3.14);

    // Verify decay for arrays (const char[N] -> const char*)
    int x = 5;
    auto p2 = mystl::make_pair(x, "test_string");
    static_assert(std::is_same_v<decltype(p2.second), const char*>, "string literal should decay to const char*");
    
    EXPECT_EQ(p2.first, 5);
    EXPECT_STREQ(p2.second, "test_string");
}

// ============================================================================
// 3. Comparison operators (Lexicographic)
// ============================================================================

TEST(PairTest, Comparisons)
{
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 2);
    Pair<int, int> p3(1, 3);
    Pair<int, int> p4(0, 5);

    // Equality
    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 != p2);
    
    // Less/Greater (first element equal, compare second)
    EXPECT_TRUE(p1 != p3);
    EXPECT_TRUE(p1 < p3);
    EXPECT_TRUE(p3 > p1);
    EXPECT_TRUE(p1 <= p3);
    EXPECT_TRUE(p3 >= p1);
    
    // Less/Greater (first element differs)
    EXPECT_TRUE(p4 < p1); // 0 < 1
    EXPECT_TRUE(p1 > p4);
    EXPECT_TRUE(p4 <= p1);
}

// ============================================================================
// 4. Swap
// ============================================================================

TEST(PairTest, Swap)
{
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(3, 4);
    
    // Member method
    p1.swap(p2);
    EXPECT_EQ(p1.first, 3);
    EXPECT_EQ(p1.second, 4);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
    
    // Free function (ADL)
    mystl::swap(p1, p2);
    EXPECT_EQ(p1.first, 1);
    EXPECT_EQ(p1.second, 2);
    EXPECT_EQ(p2.first, 3);
    EXPECT_EQ(p2.second, 4);
}