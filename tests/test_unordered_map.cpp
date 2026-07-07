#include <gtest/gtest.h>
#include <string>
#include <cstddef>
#include <new>
#include "mystl/unordered_map.hpp"

using namespace mystl;


TEST(UnorderedMapTest, DefaultConstructor) 
{
    UnorderedMap<int, std::string> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(UnorderedMapTest, InsertAndFind) 
{
    UnorderedMap<int, std::string> map;
    auto res1 = map.insert({1, "HydraEngine"});
    auto res2 = map.insert({2, "OpenGL"});

    EXPECT_TRUE(res1.second);
    EXPECT_EQ(map.size(), 2);

    auto it = map.find(1);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, "HydraEngine");

    // Re-inserting the same key should fail
    auto res3 = map.insert({1, "Fake"});
    EXPECT_FALSE(res3.second);
    EXPECT_EQ(map.size(), 2);
}

TEST(UnorderedMapTest, BracketOperator) 
{
    UnorderedMap<std::string, int> map;
    map["RenderThreads"] = 4;
    map["WindowWidth"] = 1920;

    EXPECT_EQ(map["RenderThreads"], 4);
    EXPECT_EQ(map.at("WindowWidth"), 1920);

    // Accessing a non-existent key via [] should create a default element
    EXPECT_EQ(map["FPS"], 0); 
    EXPECT_EQ(map.size(), 3);
}

TEST(UnorderedMapTest, EraseElements) 
{
    UnorderedMap<int, char> map;
    map[10] = 'a';
    map[20] = 'b';
    map[30] = 'c';

    EXPECT_EQ(map.erase(20), 1);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find(20), map.end());

    // Deleting a non-existent key
    EXPECT_EQ(map.erase(99), 0);
}

TEST(UnorderedMapTest, MoveSemantics) 
{
    UnorderedMap<int, std::string> map1;
    map1[1] = "Data";

    UnorderedMap<int, std::string> map2 = mystl::move(map1);

    EXPECT_TRUE(map1.empty());
    EXPECT_EQ(map1.bucket_count(), 0);
    
    EXPECT_EQ(map2.size(), 1);
    EXPECT_EQ(map2[1], "Data");
}

TEST(UnorderedMapTest, IteratorIteration) 
{
    UnorderedMap<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    int sum_keys = 0;
    int sum_vals = 0;

    for (auto& pair : map) 
    {
        sum_keys += pair.first;
        sum_vals += pair.second;
    }

    EXPECT_EQ(sum_keys, 6);
    EXPECT_EQ(sum_vals, 60);
}

// ============================================================================
// ALLOCATOR-EXTENDED MOVE: PROPAGATION CORRECTNESS (Blocker #3, Part 2)
// ============================================================================
namespace
{
    // Per-id outstanding-bytes ledger so a free through the wrong allocator id
    // leaves a detectable imbalance.
    struct AllocLedger
    {
        static inline long long outstanding[8] = {};
        static void      reset()      { for (auto& x : outstanding) x = 0; }
        static long long total()      { long long s = 0; for (auto x : outstanding) s += x; return s; }
    };

    // Stateful (not always-equal: it has a data member), equality by id.
    template <typename T>
    struct CountingAlloc
    {
        using value_type = T;
        int id = 0;

        template <typename U> struct rebind { using other = CountingAlloc<U>; };

        CountingAlloc() = default;
        explicit CountingAlloc(int i) : id(i) {}
        template <typename U> CountingAlloc(const CountingAlloc<U>& o) : id(o.id) {}

        T* allocate(size_t n)
        {
            AllocLedger::outstanding[id] += static_cast<long long>(n * sizeof(T));
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        void deallocate(T* p, size_t n) noexcept
        {
            AllocLedger::outstanding[id] -= static_cast<long long>(n * sizeof(T));
            ::operator delete(p);
        }
    };

    template <typename T, typename U>
    bool operator==(const CountingAlloc<T>& a, const CountingAlloc<U>& b) noexcept { return a.id == b.id; }
    template <typename T, typename U>
    bool operator!=(const CountingAlloc<T>& a, const CountingAlloc<U>& b) noexcept { return !(a == b); }

    using CountedMap = UnorderedMap<int, int, mystl::hash<int>, mystl::equal_to,
                                    CountingAlloc<mystl::Pair<const int, int>>>;
}

TEST(UnorderedMapAllocTest, AllocExtendedMoveStealsWithEqualAllocator)
{
    // Sanity: this allocator is genuinely stateful (exercises the runtime path).
    static_assert(!mystl::allocator_traits<CountingAlloc<int>>::is_always_equal::value,
                  "CountingAlloc must not be always-equal");

    AllocLedger::reset();
    {
        CountedMap src; // default allocator id == 0
        src[1] = 10;
        src[2] = 20;
        src[3] = 30;

        // Destination allocator id 0 == source id 0  => O(1) steal.
        CountedMap dst(mystl::move(src), CountingAlloc<mystl::Pair<const int, int>>(0));

        EXPECT_EQ(dst.size(), 3u);
        EXPECT_EQ(dst.at(1), 10);
        EXPECT_EQ(dst.at(2), 20);
        EXPECT_EQ(dst.at(3), 30);
        EXPECT_TRUE(src.empty());
    }
    EXPECT_EQ(AllocLedger::total(), 0); // no leak
    EXPECT_EQ(AllocLedger::outstanding[0], 0);
}

TEST(UnorderedMapAllocTest, AllocExtendedMoveMovesElementsWithUnequalAllocator)
{
    AllocLedger::reset();
    {
        CountedMap src; // allocator id 0
        src[1] = 10;
        src[2] = 20;
        src[3] = 30;

        // Destination allocator id 2 != source id 0 => element-wise move into
        // storage owned by id 2. Source's id-0 storage must be freed via id 0.
        CountedMap dst(mystl::move(src), CountingAlloc<mystl::Pair<const int, int>>(2));

        EXPECT_EQ(dst.size(), 3u);
        EXPECT_EQ(dst.at(1), 10);
        EXPECT_EQ(dst.at(2), 20);
        EXPECT_EQ(dst.at(3), 30);
        EXPECT_TRUE(src.empty());

        // dst allocated fresh storage under id 2 (elements were moved, not stolen).
        EXPECT_GT(AllocLedger::outstanding[2], 0);
        // src still legitimately owns its (now element-less) bucket array under id 0;
        // its nodes were already released through id 0 by the element-move.
        EXPECT_GT(AllocLedger::outstanding[0], 0);
    }
    // After both are destroyed everything is balanced *per id*: had dst stolen
    // src's id-0 storage and freed it through id 2, these would not both be zero.
    EXPECT_EQ(AllocLedger::outstanding[0], 0);
    EXPECT_EQ(AllocLedger::outstanding[2], 0);
    EXPECT_EQ(AllocLedger::total(), 0);
}
TEST(UnorderedMapTest, HeterogeneousIteratorComparison)
{
    UnorderedMap<int, int> m;
    m[1] = 10;
    m[2] = 20;
    m[3] = 30;

    UnorderedMap<int, int>::iterator       it  = m.begin();
    UnorderedMap<int, int>::const_iterator cit = m.cbegin();

    // Both operand orders must compile and agree.
    EXPECT_TRUE(it == cit);
    EXPECT_TRUE(cit == it);
    EXPECT_FALSE(it != cit);
    EXPECT_FALSE(cit != it);

    // Mutable iterator compared against a stored const end() while iterating.
    int visited = 0;
    for (UnorderedMap<int, int>::iterator i = m.begin(); i != m.cend(); ++i)
        ++visited;
    EXPECT_EQ(visited, 3);
}
