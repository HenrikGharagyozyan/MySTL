#include <gtest/gtest.h>
#include <string>
#include "mystl/unordered_multimap.hpp"

using namespace mystl;

TEST(UnorderedMultiMapTest, InsertionAndValueModification) 
{
    UnorderedMultiMap<int, std::string> mmap;
    
    mmap.insert({1, "SystemA"});
    mmap.insert({1, "SystemB"});
    
    EXPECT_EQ(mmap.size(), 2);
    EXPECT_EQ(mmap.count(1), 2);

    // Modify the value through a non-const iterator (allowed in a MultiMap)
    auto it = mmap.find(1);
    ASSERT_NE(it, mmap.end());
    it->second = "ModifiedSystem";

    // Check that the modification was applied
    auto new_it = mmap.find(1);
    EXPECT_EQ(new_it->second, "ModifiedSystem");
}

TEST(UnorderedMultiMapTest, EqualRangeProcessing) 
{
    UnorderedMultiMap<std::string, int> event_system;
    
    // Simulate an event system: attach 3 different listeners to one click event (function IDs)
    event_system.insert({"OnClick", 101});
    event_system.insert({"OnHover", 202});
    event_system.insert({"OnClick", 102});
    event_system.insert({"OnClick", 103});

    auto range = event_system.equal_range("OnClick");
    int sum_ids = 0;
    int count = 0;

    for (auto it = range.first; it != range.second; ++it) 
    {
        EXPECT_EQ(it->first, "OnClick");
        sum_ids += it->second;
        count++;
    }

    EXPECT_EQ(count, 3);
    EXPECT_EQ(sum_ids, 101 + 102 + 103); // 306
}

TEST(UnorderedMultiMapTest, EraseAllKeys) 
{
    UnorderedMultiMap<int, char> mmap;
    mmap.insert({5, 'x'});
    mmap.insert({5, 'y'});
    mmap.insert({3, 'z'});

    size_t erased = mmap.erase(5);
    EXPECT_EQ(erased, 2);
    EXPECT_EQ(mmap.size(), 1);
    EXPECT_EQ(mmap.count(5), 0);
}