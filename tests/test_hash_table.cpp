#include <gtest/gtest.h>
#include <string>

#include "mystl/hash_table.hpp"

using namespace mystl;

// Set-like configuration: Value == Key, extractor == Identity.
using IntSetTable = HashTable<int, int, Identity<int>, mystl::hash<int>, mystl::equal_to, mystl::Allocator<int>>;

// Map-like configuration: Value == Pair<const Key, T>, extractor == Select1st.
using IntMapValue = mystl::Pair<const int, std::string>;
using IntMapTable = HashTable<int, IntMapValue, Select1st<IntMapValue>,
                              mystl::hash<int>, mystl::equal_to,
                              mystl::Allocator<IntMapValue>>;

// ============================================================================
// UNIQUE INSERT (set-like)
// ============================================================================
TEST(HashTableTest, EmplaceUniqueDeduplicates)
{
    IntSetTable t;
    auto r1 = t.emplace_unique(5);
    auto r2 = t.emplace_unique(5);

    EXPECT_TRUE(r1.second);
    EXPECT_FALSE(r2.second);
    EXPECT_EQ(r1.first, r2.first);
    EXPECT_EQ(t.size(), 1u);
    EXPECT_EQ(t.count(5), 1u);
    EXPECT_TRUE(t.contains(5));
    EXPECT_FALSE(t.contains(6));
}

TEST(HashTableTest, FindAndEraseUnique)
{
    IntSetTable t;
    for (int i = 0; i < 20; ++i) t.emplace_unique(i);
    EXPECT_EQ(t.size(), 20u);

    EXPECT_NE(t.find(7), t.end());
    EXPECT_EQ(t.erase(7), 1u);
    EXPECT_EQ(t.erase(7), 0u);
    EXPECT_EQ(t.find(7), t.end());
    EXPECT_EQ(t.size(), 19u);
}

TEST(HashTableTest, RehashPreservesAllElements)
{
    IntSetTable t(2); // small, forces rehashing
    for (int i = 0; i < 100; ++i) t.emplace_unique(i);

    EXPECT_EQ(t.size(), 100u);
    EXPECT_GT(t.bucket_count(), 2u);
    for (int i = 0; i < 100; ++i)
        EXPECT_TRUE(t.contains(i)) << "missing " << i;

    // Full iteration visits exactly 100 distinct elements.
    int seen = 0;
    for (auto it = t.begin(); it != t.end(); ++it) ++seen;
    EXPECT_EQ(seen, 100);
}

// ============================================================================
// MULTI INSERT (set-like)
// ============================================================================
TEST(HashTableTest, EmplaceMultiKeepsDuplicatesAdjacent)
{
    IntSetTable t;
    t.emplace_multi(5);
    t.emplace_multi(5);
    t.emplace_multi(5);
    t.emplace_multi(9);

    EXPECT_EQ(t.size(), 4u);
    EXPECT_EQ(t.count(5), 3u);
    EXPECT_EQ(t.count(9), 1u);

    auto range = t.equal_range(5);
    int in_range = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(*it, 5);
        ++in_range;
    }
    EXPECT_EQ(in_range, 3);
}

TEST(HashTableTest, EraseMultiRemovesAllEquivalent)
{
    IntSetTable t;
    t.emplace_multi(1);
    t.emplace_multi(2);
    t.emplace_multi(2);
    t.emplace_multi(2);
    t.emplace_multi(3);

    EXPECT_EQ(t.erase(2), 3u);
    EXPECT_EQ(t.count(2), 0u);
    EXPECT_EQ(t.size(), 2u);
}

// ============================================================================
// MAP-LIKE CONFIGURATION
// ============================================================================
TEST(HashTableTest, MapLikeUniqueAndMutableValue)
{
    IntMapTable t;
    t.emplace_unique(1, std::string("one"));
    t.emplace_unique(2, std::string("two"));
    auto dup = t.emplace_unique(1, std::string("ONE"));
    EXPECT_FALSE(dup.second); // key already present

    auto it = t.find(1);
    ASSERT_NE(it, t.end());
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");

    // Mutable iterator lets us change the mapped value (but not the key).
    it->second = "uno";
    EXPECT_EQ(t.find(1)->second, "uno");
    EXPECT_EQ(t.size(), 2u);
}

TEST(HashTableTest, MapLikeMultiEqualRange)
{
    IntMapTable t;
    t.emplace_multi(7, std::string("a"));
    t.emplace_multi(7, std::string("b"));
    t.emplace_multi(8, std::string("c"));

    EXPECT_EQ(t.count(7), 2u);
    auto range = t.equal_range(7);
    int n = 0;
    for (auto it = range.first; it != range.second; ++it)
    {
        EXPECT_EQ(it->first, 7);
        ++n;
    }
    EXPECT_EQ(n, 2);
}

// ============================================================================
// RULE OF FIVE
// ============================================================================
TEST(HashTableTest, CopyIsIndependentDeepCopy)
{
    IntSetTable a;
    for (int i = 0; i < 10; ++i) a.emplace_unique(i);

    IntSetTable b(a);
    EXPECT_EQ(b.size(), 10u);
    for (int i = 0; i < 10; ++i) EXPECT_TRUE(b.contains(i));

    a.erase(0);
    EXPECT_FALSE(a.contains(0));
    EXPECT_TRUE(b.contains(0)); // copy unaffected
}

TEST(HashTableTest, MoveTransfersOwnership)
{
    IntSetTable a;
    for (int i = 0; i < 10; ++i) a.emplace_unique(i);

    IntSetTable b(mystl::move(a));
    EXPECT_EQ(b.size(), 10u);
    EXPECT_TRUE(a.empty());
    for (int i = 0; i < 10; ++i) EXPECT_TRUE(b.contains(i));
}

TEST(HashTableTest, CopyAndMoveAssignment)
{
    IntSetTable a;
    for (int i = 0; i < 5; ++i) a.emplace_unique(i);

    IntSetTable b;
    b = a;
    EXPECT_EQ(b.size(), 5u);

    IntSetTable c;
    c = mystl::move(b);
    EXPECT_EQ(c.size(), 5u);
    EXPECT_TRUE(b.empty());
    for (int i = 0; i < 5; ++i) EXPECT_TRUE(c.contains(i));
}

TEST(HashTableTest, SwapExchangesContents)
{
    IntSetTable a; a.emplace_unique(1); a.emplace_unique(2);
    IntSetTable b; b.emplace_unique(9);

    a.swap(b);
    EXPECT_EQ(a.size(), 1u);
    EXPECT_TRUE(a.contains(9));
    EXPECT_EQ(b.size(), 2u);
    EXPECT_TRUE(b.contains(1));
    EXPECT_TRUE(b.contains(2));
}
