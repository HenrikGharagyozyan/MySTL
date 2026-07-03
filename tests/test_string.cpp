#include <gtest/gtest.h>
#include "../include/mystl/string.hpp"

TEST(StringTest, SSOToHeapTransition) 
{
    mystl::String s;

    for(int i = 0; i < 15; ++i) 
        s.push_back('a');
    EXPECT_EQ(s.size(), 15);
    EXPECT_LE(s.capacity(), 15);

    s.push_back('b');
    EXPECT_EQ(s.size(), 16);
    EXPECT_GT(s.capacity(), 15);
    EXPECT_EQ(s[15], 'b');
}

TEST(StringTest, IteratorAndRangeBasedFor) 
{
    mystl::String s("Hello");
    std::string check;
    for(auto c : s) 
        check += c;

    EXPECT_EQ(check, "Hello");
}

TEST(StringTest, MoveSemantics) 
{
    mystl::String s1("Longer string to force heap allocation");
    mystl::String s2 = std::move(s1);
    
    EXPECT_EQ(s2, "Longer string to force heap allocation");
    EXPECT_EQ(s1.size(), 0); 
}
TEST(StringTest, OrderingIsUnsignedForHighBytes)
{
    // Byte 0xFF (255) must order AFTER 'a' (0x61), not before it as signed
    // char would (-1 < 97). This matches std::string / char_traits semantics.
    mystl::String high("\xff");
    mystl::String low("a");

    EXPECT_TRUE(low < high);
    EXPECT_FALSE(high < low);
    EXPECT_TRUE(high > low);
    EXPECT_TRUE(low <= high);
    EXPECT_TRUE(high >= low);

    EXPECT_LT(low.compare(high), 0);
    EXPECT_GT(high.compare(low), 0);
}

TEST(StringTest, OrderingAsciiRegression)
{
    mystl::String a("apple");
    mystl::String b("banana");
    mystl::String a2("apple");

    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
    EXPECT_EQ(a.compare(a2), 0);
    EXPECT_TRUE(a <= a2);
    EXPECT_TRUE(a >= a2);

    // Prefix orders before the longer string.
    mystl::String app("app");
    EXPECT_TRUE(app < a);
    EXPECT_LT(app.compare(a), 0);
}
