#include <gtest/gtest.h>
#include "mystl/forward_list.hpp"
#include "mystl/string.hpp"

TEST(ForwardListTest, DefaultConstruction) 
{
    mystl::ForwardList<int> flist;
    EXPECT_TRUE(flist.empty());
    EXPECT_EQ(flist.begin(), flist.end());
}

TEST(ForwardListTest, PushAndPopFront) 
{
    mystl::ForwardList<int> flist;
    flist.push_front(10);
    flist.push_front(20);
    flist.push_front(30);

    // Expected order: 30 -> 20 -> 10
    EXPECT_FALSE(flist.empty());
    EXPECT_EQ(flist.front(), 30);

    flist.pop_front();
    EXPECT_EQ(flist.front(), 20);

    flist.pop_front();
    EXPECT_EQ(flist.front(), 10);

    flist.pop_front();
    EXPECT_TRUE(flist.empty());
}

TEST(ForwardListTest, InsertAndEraseAfter) 
{
    mystl::ForwardList<int> flist;
    
    // Insert at the very beginning via before_begin()
    auto it = flist.insert_after(flist.before_begin(), 1);
    
    // Insert after the first element
    flist.insert_after(it, 2);

    // Expect: 1 -> 2
    EXPECT_EQ(flist.front(), 1);

    // Remove the element after before_begin (remove 1)
    flist.erase_after(flist.before_begin());
    
    EXPECT_EQ(flist.front(), 2);
}

TEST(ForwardListTest, CopyAndMoveSemantics) 
{
    mystl::ForwardList<int> flist1 = {1, 2, 3};
    
    // Copying
    mystl::ForwardList<int> flist2 = flist1;
    EXPECT_EQ(flist1, flist2);

    // Modifying the copy
    flist2.push_front(0);
    EXPECT_NE(flist1, flist2);

    // Moving
    mystl::ForwardList<int> flist3 = mystl::move(flist1);
    EXPECT_TRUE(flist1.empty());
    
    auto it = flist3.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it, 3);
}

TEST(ForwardListTest, ComplexObjects) 
{
    mystl::ForwardList<mystl::String> flist;
    flist.emplace_after(flist.before_begin(), "Hello");
    flist.push_front("World");
    
    // World -> Hello
    EXPECT_EQ(flist.front(), "World");
}