#include <gtest/gtest.h>
#include "mystl/stack.hpp"
#include "mystl/string.hpp"

TEST(StackTest, DefaultConstruction) 
{
    mystl::Stack<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST(StackTest, PushPopTop) 
{
    mystl::Stack<int> s;
    s.push(10);
    s.push(20);
    s.push(30);

    // The order should be: 30 (top), 20, 10
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 30);

    s.pop();
    EXPECT_EQ(s.top(), 20);
    EXPECT_EQ(s.size(), 2);
    
    s.pop();
    EXPECT_EQ(s.top(), 10);
    EXPECT_EQ(s.size(), 1);
}

TEST(StackTest, EmplaceAndMove) 
{
    mystl::Stack<mystl::String> s;
    s.emplace("Hydra Engine"); // Uses emplace_back from Deque
    
    EXPECT_EQ(s.top(), "Hydra Engine");

    mystl::Stack<mystl::String> s2 = std::move(s);
    EXPECT_EQ(s2.top(), "Hydra Engine");
    EXPECT_EQ(s2.size(), 1);
    EXPECT_TRUE(s.empty());
}

TEST(StackTest, Swap) 
{
    mystl::Stack<int> s1;
    s1.push(1);
    s1.push(2);

    mystl::Stack<int> s2;
    s2.push(100);

    s1.swap(s2);

    EXPECT_EQ(s1.size(), 1);
    EXPECT_EQ(s1.top(), 100);

    EXPECT_EQ(s2.size(), 2);
    EXPECT_EQ(s2.top(), 2);
}