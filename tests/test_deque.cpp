#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <vector>
#include "mystl/deque.hpp"
#include "mystl/string.hpp"
#include "mystl/utility.hpp" // If you have mystl::move implemented there

// Test structure for strict memory leak checking
struct Tracker 
{
    static int instances;
    int id;

    Tracker(int i = 0) : id(i) { ++instances; }
    Tracker(const Tracker& other) : id(other.id) { ++instances; }
    ~Tracker() { --instances; }
    
    bool operator==(const Tracker& other) const { return id == other.id; }
};
int Tracker::instances = 0;

template <typename T>
struct StatefulAllocator
{
    using value_type = T;
    using pointer = T*;
    using size_type = size_t;

    int id = 0;

    StatefulAllocator() noexcept = default;
    explicit StatefulAllocator(int allocator_id) noexcept : id(allocator_id) {}

    template <typename U>
    StatefulAllocator(const StatefulAllocator<U>& other) noexcept : id(other.id) {}

    pointer allocate(size_type n)
    {
        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type) noexcept
    {
        ::operator delete(p);
    }

    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        ::new (static_cast<void*>(p)) U(mystl::forward<Args>(args)...);
    }

    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }
};

template <typename T, typename U>
bool operator==(const StatefulAllocator<T>& lhs, const StatefulAllocator<U>& rhs) noexcept
{
    return lhs.id == rhs.id;
}

template <typename T, typename U>
bool operator!=(const StatefulAllocator<T>& lhs, const StatefulAllocator<U>& rhs) noexcept
{
    return !(lhs == rhs);
}

static_assert(std::uses_allocator<mystl::Deque<int, StatefulAllocator<int>>,
                                  StatefulAllocator<int>>::value,
              "Deque must participate in uses_allocator");

TEST(DequeTest, DefaultConstruction) 
{
    mystl::Deque<int> d;
    EXPECT_TRUE(d.empty());
    EXPECT_EQ(d.size(), 0);
}

TEST(DequeTest, PushAndPop) 
{
    mystl::Deque<int> d;
    
    // Check push_back and allocation of new blocks
    for (int i = 0; i < 1000; ++i) 
    {
        d.push_back(i);
    }
    EXPECT_EQ(d.size(), 1000);
    EXPECT_EQ(d.front(), 0);
    EXPECT_EQ(d.back(), 999);

    // Check push_front and block allocation at the beginning
    for (int i = 1; i <= 500; ++i) 
    {
        d.push_front(-i);
    }
    EXPECT_EQ(d.size(), 1500);
    EXPECT_EQ(d.front(), -500);

    // Check pop_front
    for (int i = 0; i < 500; ++i) 
    {
        d.pop_front();
    }
    EXPECT_EQ(d.front(), 0);

    // Check pop_back
    for (int i = 0; i < 500; ++i) 
    {
        d.pop_back();
    }
    EXPECT_EQ(d.size(), 500);
    EXPECT_EQ(d.back(), 499);
}

TEST(DequeTest, IteratorsAndElementAccess) 
{
    mystl::Deque<int> d = {10, 20, 30, 40, 50};
    
    int sum = 0;
    for (int val : d) 
    {
        sum += val;
    }
    EXPECT_EQ(sum, 150);

    // O(1) iterator arithmetic
    auto it = d.begin();
    EXPECT_EQ(*(it + 2), 30);
    EXPECT_EQ(*(d.end() - 1), 50);
    
    // Direct access
    EXPECT_EQ(d[1], 20);
    EXPECT_EQ(d.at(3), 40);
    EXPECT_THROW(d.at(10), std::out_of_range);
}

TEST(DequeTest, MemoryManagement) 
{
    Tracker::instances = 0; // Reset the counter before the test
    {
        mystl::Deque<Tracker> d;
        for (int i = 0; i < 1000; ++i) 
        {
            d.emplace_back(i);
        }
        EXPECT_EQ(Tracker::instances, 1000);
        
        d.pop_front();
        d.pop_back();
        EXPECT_EQ(Tracker::instances, 998); // 2 objects should be destroyed
        
        d.clear();
        EXPECT_EQ(Tracker::instances, 0); // Memory is fully cleaned up
    }
    EXPECT_EQ(Tracker::instances, 0); // Check after leaving the scope
}

TEST(DequeTest, CopyAndMoveSemantics) 
{
    mystl::Deque<int> d1 = {1, 2, 3};
    mystl::Deque<int> d2 = d1; // Copy constructor

    EXPECT_EQ(d2.size(), 3);
    EXPECT_EQ(d2[0], 1);
    
    mystl::Deque<int> d3 = std::move(d1); // Move constructor
    EXPECT_EQ(d3.size(), 3);
    EXPECT_EQ(d3[2], 3);
    
    EXPECT_EQ(d1.size(), 0); // The original should become empty after move
    EXPECT_TRUE(d1.empty());
}

TEST(DequeTest, AllocatorConstruction)
{
    using Alloc = StatefulAllocator<int>;

    mystl::Deque<int, Alloc> d(Alloc(11));
    EXPECT_EQ(d.get_allocator().id, 11);

    d.push_back(10);
    d.push_front(5);
    EXPECT_EQ(d.front(), 5);
    EXPECT_EQ(d.back(), 10);
}

TEST(DequeTest, RangeConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;

    std::vector<int> values = {1, 2, 3, 4};
    mystl::Deque<int, Alloc> d(values.begin(), values.end(), Alloc(12));

    EXPECT_EQ(d.get_allocator().id, 12);
    EXPECT_EQ(d.size(), 4);
    EXPECT_EQ(d.front(), 1);
    EXPECT_EQ(d.back(), 4);
}

TEST(DequeTest, CopyAndMoveConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;

    mystl::Deque<int, Alloc> source({1, 2, 3}, Alloc(1));
    mystl::Deque<int, Alloc> copy(source, Alloc(2));

    EXPECT_EQ(copy.get_allocator().id, 2);
    EXPECT_EQ(copy.size(), 3);
    EXPECT_EQ(copy[2], 3);

    mystl::Deque<int, Alloc> moved(mystl::move(source), Alloc(3));
    EXPECT_EQ(moved.get_allocator().id, 3);
    EXPECT_EQ(moved.size(), 3);
    EXPECT_EQ(moved.front(), 1);
}

// The (count, value) constructor must not be hijacked by the (InputIt, InputIt)
// constructor when both arguments are integral.
TEST(DequeTest, CountValueConstructionNotHijacked)
{
    mystl::Deque<int> d(5, 10);

    EXPECT_EQ(d.size(), 5u);
    for (size_t i = 0; i < d.size(); ++i)
        EXPECT_EQ(d[i], 10);
}

TEST(DequeTest, CountValueWithNonIntegralValueType)
{
    // Two integral-looking args resolve to (count, value) here too.
    mystl::Deque<char> d(3, 'z');
    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d[0], 'z');
    EXPECT_EQ(d[2], 'z');
}

TEST(DequeTest, IteratorPairConstructionStillWorks)
{
    // Genuine iterator pairs (non-integral) must still select the range ctor.
    std::vector<int> src = {7, 8, 9};
    mystl::Deque<int> d(src.begin(), src.end());

    EXPECT_EQ(d.size(), 3u);
    EXPECT_EQ(d.front(), 7);
    EXPECT_EQ(d.back(), 9);
}
