#include <gtest/gtest.h>
#include <vector>
#include <string>

#include "mystl/multiset.hpp"

// ============================================================================
// CONSTRUCTOR TESTS
// ============================================================================
TEST(MultiSetTest, DefaultConstructor) 
{
    mystl::MultiSet<int> mset;
    EXPECT_TRUE(mset.empty());
    EXPECT_EQ(mset.size(), 0);
}

TEST(MultiSetTest, InitializerListConstructor) 
{
    mystl::MultiSet<int> mset = {5, 2, 8, 2, 5, 5};
    EXPECT_FALSE(mset.empty());
    EXPECT_EQ(mset.size(), 6);
    
    // Elements should be ordered: 2, 2, 5, 5, 5, 8
    auto it = mset.begin();
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 5);
    EXPECT_EQ(*it++, 8);
    EXPECT_EQ(it, mset.end());
}

TEST(MultiSetTest, CopyConstructor) 
{
    mystl::MultiSet<int> original = {10, 20, 10, 30};
    mystl::MultiSet<int> copy(original);
    
    EXPECT_EQ(copy.size(), 4);
    EXPECT_TRUE(copy == original);
}

TEST(MultiSetTest, MoveConstructor) 
{
    mystl::MultiSet<std::string> original = {"apple", "banana", "apple"};
    mystl::MultiSet<std::string> moved(std::move(original));
    
    EXPECT_EQ(moved.size(), 3);
    EXPECT_EQ(moved.count("apple"), 2);
    EXPECT_TRUE(original.empty());
}

// ============================================================================
// MODIFIER TESTS
// ============================================================================
TEST(MultiSetTest, InsertSingleElement) 
{
    mystl::MultiSet<int> mset;
    auto it1 = mset.insert(42);
    EXPECT_EQ(*it1, 42);
    EXPECT_EQ(mset.size(), 1);

    auto it2 = mset.insert(42); // Should allow duplicate
    EXPECT_EQ(*it2, 42);
    EXPECT_EQ(mset.size(), 2);
    EXPECT_NE(it1, it2); // Iterators to different nodes
}

TEST(MultiSetTest, Emplace) 
{
    mystl::MultiSet<std::string> mset;
    mset.emplace("hello");
    mset.emplace(5, 'A'); // Constructs "AAAAA"
    mset.emplace("hello");

    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count("hello"), 2);
    EXPECT_EQ(mset.count("AAAAA"), 1);
}

TEST(MultiSetTest, EraseByKey) 
{
    mystl::MultiSet<int> mset = {1, 2, 2, 2, 3, 4, 4};
    
    size_t erased_count = mset.erase(2);
    EXPECT_EQ(erased_count, 3);
    EXPECT_EQ(mset.size(), 4);
    EXPECT_EQ(mset.count(2), 0);
    
    erased_count = mset.erase(99); // Key not present
    EXPECT_EQ(erased_count, 0);
    EXPECT_EQ(mset.size(), 4);
}

TEST(MultiSetTest, EraseByIterator) 
{
    mystl::MultiSet<int> mset = {10, 20, 20, 30};
    
    auto it = mset.find(20);
    EXPECT_NE(it, mset.end());
    
    auto next_it = mset.erase(it); // Erase only one '20'
    EXPECT_EQ(*next_it, 20);
    EXPECT_EQ(mset.size(), 3);
    EXPECT_EQ(mset.count(20), 1); // One should still remain
}

TEST(MultiSetTest, Clear) 
{
    mystl::MultiSet<int> mset = {1, 1, 1, 2, 3};
    EXPECT_FALSE(mset.empty());
    
    mset.clear();
    EXPECT_TRUE(mset.empty());
    EXPECT_EQ(mset.size(), 0);
    EXPECT_EQ(mset.begin(), mset.end());
}

// ============================================================================
// SEARCH AND OBSERVER TESTS
// ============================================================================
TEST(MultiSetTest, FindAndContains) 
{
    mystl::MultiSet<int> mset = {5, 10, 10, 15};
    
    EXPECT_TRUE(mset.contains(10));
    EXPECT_FALSE(mset.contains(20));
    
    auto it = mset.find(10);
    EXPECT_NE(it, mset.end());
    EXPECT_EQ(*it, 10);
    
    EXPECT_EQ(mset.find(99), mset.end());
}

TEST(MultiSetTest, Count) 
{
    mystl::MultiSet<int> mset = {7, 7, 7, 8, 9, 9};
    
    EXPECT_EQ(mset.count(7), 3);
    EXPECT_EQ(mset.count(8), 1);
    EXPECT_EQ(mset.count(9), 2);
    EXPECT_EQ(mset.count(10), 0);
}

TEST(MultiSetTest, BoundsAndEqualRange) 
{
    mystl::MultiSet<int> mset = {10, 20, 20, 20, 30};
    
    auto lower = mset.lower_bound(20);
    auto upper = mset.upper_bound(20);
    
    // lower_bound should point to the first 20
    EXPECT_EQ(*lower, 20);
    
    // upper_bound should point to the first element > 20 (which is 30)
    EXPECT_EQ(*upper, 30);
    
    // equal_range should return a pair of [lower_bound, upper_bound]
    auto range = mset.equal_range(20);
    EXPECT_EQ(range.first, lower);
    EXPECT_EQ(range.second, upper);
    
    // The distance between lower and upper should match the count
    int count = 0;
    for (auto it = range.first; it != range.second; ++it) 
    {
        count++;
    }
    EXPECT_EQ(count, 3);
}

// ============================================================================
// ALGORITHM / ITERATOR TESTS
// ============================================================================
TEST(MultiSetTest, ReverseIterators) 
{
    mystl::MultiSet<int> mset = {1, 2, 2, 3};
    std::vector<int> result;
    
    for (auto it = mset.rbegin(); it != mset.rend(); ++it) 
    {
        result.push_back(*it);
    }
    
    std::vector<int> expected = {3, 2, 2, 1};
    EXPECT_EQ(result, expected);
}