#include <gtest/gtest.h>
#include "mystl/list.hpp"
#include "mystl/string.hpp"

TEST(ListTest, DefaultConstruction) 
{
    mystl::List<int> lst;
    EXPECT_TRUE(lst.empty());
    EXPECT_EQ(lst.size(), 0);
}

TEST(ListTest, PushAndPop) 
{
    mystl::List<int> lst;
    lst.push_back(10);
    lst.push_front(20);
    lst.push_back(30);

    // 20 <-> 10 <-> 30
    EXPECT_EQ(lst.size(), 3);
    EXPECT_EQ(lst.front(), 20);
    EXPECT_EQ(lst.back(), 30);

    lst.pop_front();
    EXPECT_EQ(lst.front(), 10);
    
    lst.pop_back();
    EXPECT_EQ(lst.back(), 10);
    EXPECT_EQ(lst.size(), 1);
}

TEST(ListTest, Iterators) 
{
    mystl::List<int> lst = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int val : lst) 
    {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST(ListTest, CopySemantics) 
{
    mystl::List<int> lst1 = {10, 20, 30};
    mystl::List<int> lst2 = lst1; // Copy constructor

    EXPECT_EQ(lst2.size(), 3);
    EXPECT_EQ(lst2, lst1); // Uses our operator==

    lst2.push_back(40);
    EXPECT_NE(lst1, lst2); // Changing the copy does not affect the original
}

TEST(ListTest, MoveSemantics) 
{
    mystl::List<mystl::String> lst1;
    lst1.push_back("Hello");
    lst1.push_back("World");

    mystl::List<mystl::String> lst2 = mystl::move(lst1); // Move constructor

    EXPECT_EQ(lst1.size(), 0); // The original is empty
    EXPECT_TRUE(lst1.empty());
    
    EXPECT_EQ(lst2.size(), 2);
    EXPECT_EQ(lst2.front(), "Hello");
}

TEST(ListTest, InsertAndErase) 
{
    mystl::List<int> lst = {1, 3};
    auto it = lst.begin();
    ++it; // Point to 3
    
    lst.insert(it, 2); // Insert 2 before 3
    EXPECT_EQ(lst.size(), 3);
    
    auto it_check = lst.begin();
    EXPECT_EQ(*it_check++, 1);
    EXPECT_EQ(*it_check++, 2);
    EXPECT_EQ(*it_check, 3);
}