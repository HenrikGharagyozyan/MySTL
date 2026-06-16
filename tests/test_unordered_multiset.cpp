#include <gtest/gtest.h>
#include <string>
#include "mystl/unordered_multiset.hpp"

using namespace mystl;

TEST(UnorderedMultiSetTest, MultipleInsertions) 
{
    UnorderedMultiSet<int> mset;
    
    // Вставка всегда успешна, возвращается только итератор
    mset.insert(42);
    mset.insert(42);
    mset.insert(42);
    
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(42), 3);
}

TEST(UnorderedMultiSetTest, EqualRange) 
{
    UnorderedMultiSet<std::string> mset;
    mset.insert("Apple");
    mset.insert("Banana");
    mset.insert("Apple");
    mset.insert("Cherry");
    mset.insert("Apple");

    auto range = mset.equal_range("Apple");
    int count = 0;
    
    for (auto it = range.first; it != range.second; ++it) 
    {
        EXPECT_EQ(*it, "Apple");
        count++;
    }
    
    EXPECT_EQ(count, 3);
    EXPECT_EQ(mset.size(), 5);
}

TEST(UnorderedMultiSetTest, EraseAllMatches) 
{
    UnorderedMultiSet<int> mset;
    mset.insert(10);
    mset.insert(10);
    mset.insert(20);

    // Должно удалиться ровно два элемента со значением 10
    size_t erased = mset.erase(10);
    
    EXPECT_EQ(erased, 2);
    EXPECT_EQ(mset.size(), 1);
    EXPECT_EQ(mset.count(10), 0);
    EXPECT_TRUE(mset.contains(20));
}

TEST(UnorderedMultiSetTest, IteratorValidity) 
{
    UnorderedMultiSet<int> mset;
    mset.insert(1);
    mset.insert(2);
    mset.insert(3);
    mset.insert(2);

    int sum = 0;
    for (const auto& val : mset) 
    {
        sum += val;
    }
    
    // 1 + 2 + 3 + 2 = 8
    EXPECT_EQ(sum, 8);
}