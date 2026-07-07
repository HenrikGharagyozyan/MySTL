#include <gtest/gtest.h>
#include <string>
#include <cstddef>
#include <new>
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

// ============================================================================
// ALLOCATOR-EXTENDED MOVE WITH UNEQUAL ALLOCATOR (Blocker #3, Part 2)
// Multi-container path: equivalent keys must remain adjacent after the
// element-wise move, and no memory may be freed through the wrong allocator.
// ============================================================================
namespace
{
    struct MMLedger
    {
        static inline long long outstanding[8] = {};
        static void      reset() { for (auto& x : outstanding) x = 0; }
        static long long total() { long long s = 0; for (auto x : outstanding) s += x; return s; }
    };

    template <typename T>
    struct MMCountingAlloc
    {
        using value_type = T;
        int id = 0;

        template <typename U> struct rebind { using other = MMCountingAlloc<U>; };

        MMCountingAlloc() = default;
        explicit MMCountingAlloc(int i) : id(i) {}
        template <typename U> MMCountingAlloc(const MMCountingAlloc<U>& o) : id(o.id) {}

        T* allocate(size_t n)
        {
            MMLedger::outstanding[id] += static_cast<long long>(n * sizeof(T));
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }
        void deallocate(T* p, size_t n) noexcept
        {
            MMLedger::outstanding[id] -= static_cast<long long>(n * sizeof(T));
            ::operator delete(p);
        }
    };

    template <typename T, typename U>
    bool operator==(const MMCountingAlloc<T>& a, const MMCountingAlloc<U>& b) noexcept { return a.id == b.id; }
    template <typename T, typename U>
    bool operator!=(const MMCountingAlloc<T>& a, const MMCountingAlloc<U>& b) noexcept { return !(a == b); }

    using CountedMMap = UnorderedMultiMap<int, int, mystl::hash<int>, mystl::equal_to,
                                          MMCountingAlloc<mystl::Pair<const int, int>>>;
}

TEST(UnorderedMultiMapAllocTest, AllocExtendedMoveUnequalPreservesMultiAndBalances)
{
    static_assert(!mystl::allocator_traits<MMCountingAlloc<int>>::is_always_equal::value,
                  "MMCountingAlloc must not be always-equal");

    MMLedger::reset();
    {
        CountedMMap src; // id 0
        src.insert({7, 1});
        src.insert({7, 2});
        src.insert({7, 3});
        src.insert({9, 4});

        // Unequal allocator id 2 => element-wise move into id-2 storage.
        CountedMMap dst(mystl::move(src), MMCountingAlloc<mystl::Pair<const int, int>>(2));

        EXPECT_EQ(dst.size(), 4u);
        EXPECT_EQ(dst.count(7), 3u); // three equivalent keys survived, still grouped
        EXPECT_EQ(dst.count(9), 1u);
        EXPECT_TRUE(src.empty());

        EXPECT_GT(MMLedger::outstanding[2], 0); // dst owns fresh id-2 storage
    }
    // Per-id balance proves nothing was freed through the wrong allocator.
    EXPECT_EQ(MMLedger::outstanding[0], 0);
    EXPECT_EQ(MMLedger::outstanding[2], 0);
    EXPECT_EQ(MMLedger::total(), 0);
}