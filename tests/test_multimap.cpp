#include <gtest/gtest.h>
#include <string>
#include <stdexcept>
#include <vector>

#include "mystl/multimap.hpp"

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================
TEST(MultiMapTest, DefaultConstructor) 
{
    mystl::multimap<int, std::string> mmap;
    EXPECT_TRUE(mmap.empty());
    EXPECT_EQ(mmap.size(), 0);
}

TEST(MultiMapTest, InitializerListConstructor) 
{
    mystl::multimap<int, std::string> mmap = {
        {1, "apple"}, 
        {2, "banana"}, 
        {1, "apricot"}, 
        {3, "cherry"}
    };
    
    EXPECT_FALSE(mmap.empty());
    EXPECT_EQ(mmap.size(), 4);
    EXPECT_EQ(mmap.count(1), 2);
}

TEST(MultiMapTest, CopyAndMoveConstructors) 
{
    mystl::multimap<int, int> original = {{1, 10}, {1, 20}, {2, 30}};
    
    mystl::multimap<int, int> copy(original);
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy.count(1), 2);

    mystl::multimap<int, int> moved(std::move(original));
    EXPECT_EQ(moved.size(), 3);
    EXPECT_EQ(moved.count(2), 1);
    EXPECT_TRUE(original.empty());
}

// ============================================================================
// MODIFIER TESTS
// ============================================================================
TEST(MultiMapTest, InsertAndEmplace) 
{
    mystl::multimap<std::string, int> mmap;
    
    // Insert single elements
    mmap.insert({"key1", 100});
    mmap.insert({"key1", 200}); // Duplicate key
    
    // Emplace
    mmap.emplace("key2", 300);
    
    EXPECT_EQ(mmap.size(), 3);
    EXPECT_EQ(mmap.count("key1"), 2);
    EXPECT_EQ(mmap.count("key2"), 1);
}

TEST(MultiMapTest, EraseByKey) 
{
    mystl::multimap<int, std::string> mmap = {
        {1, "A"}, {2, "B"}, {2, "C"}, {2, "D"}, {3, "E"}
    };
    
    size_t erased_count = mmap.erase(2);
    EXPECT_EQ(erased_count, 3);
    EXPECT_EQ(mmap.size(), 2);
    EXPECT_EQ(mmap.count(2), 0);
    
    erased_count = mmap.erase(99); // Non-existent
    EXPECT_EQ(erased_count, 0);
}

TEST(MultiMapTest, EraseByIterator) 
{
    mystl::multimap<int, std::string> mmap = {{1, "A"}, {1, "B"}, {1, "C"}};
    
    auto it = mmap.find(1); // Points to one of the '1's
    EXPECT_NE(it, mmap.end());
    
    mmap.erase(it);
    EXPECT_EQ(mmap.size(), 2);
    EXPECT_EQ(mmap.count(1), 2); // Two elements should remain
}

// ============================================================================
// ELEMENT ACCESS TESTS
// ============================================================================
TEST(MultiMapTest, AtMethod) 
{
    mystl::multimap<int, std::string> mmap = {{1, "First"}, {2, "Second"}};
    
    EXPECT_EQ(mmap.at(1), "First");
    EXPECT_EQ(mmap.at(2), "Second");
    
    // Modification via at()
    mmap.at(1) = "Modified";
    EXPECT_EQ(mmap.at(1), "Modified");
    
    // Exceptions
    EXPECT_THROW(mmap.at(99), std::out_of_range);
}

// ============================================================================
// SEARCH TESTS
// ============================================================================
TEST(MultiMapTest, BoundsAndEqualRange) 
{
    mystl::multimap<int, std::string> mmap = {
        {10, "A"}, 
        {20, "B1"}, 
        {20, "B2"}, 
        {30, "C"}
    };
    
    auto lower = mmap.lower_bound(20);
    EXPECT_EQ(lower->first, 20);
    
    auto upper = mmap.upper_bound(20);
    EXPECT_EQ(upper->first, 30);
    
    auto range = mmap.equal_range(20);
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) 
    {
        EXPECT_EQ(it->first, 20);
        count++;
    }
    EXPECT_EQ(count, 2);
}

// ============================================================================
// VALUE_COMPARE TESTS
// ============================================================================
TEST(MultiMapTest, ValueCompare) 
{
    mystl::multimap<int, std::string> mmap;
    auto val_comp = mmap.value_comp();
    
    mystl::Pair<const int, std::string> p1{1, "A"};
    mystl::Pair<const int, std::string> p2{2, "B"};
    
    EXPECT_TRUE(val_comp(p1, p2));
    EXPECT_FALSE(val_comp(p2, p1));
}