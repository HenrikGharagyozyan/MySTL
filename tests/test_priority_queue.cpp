#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <new>
#include <vector>
#include <functional>
#include "mystl/priority_queue.hpp"

using namespace mystl;

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

static_assert(std::uses_allocator<
                  PriorityQueue<int, mystl::Vector<int, StatefulAllocator<int>>>,
                  StatefulAllocator<int>>::value,
              "PriorityQueue must participate in uses_allocator");

TEST(PriorityQueueTest, DefaultConstruction) 
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

TEST(PriorityQueueTest, PushAndTopMaxHeap) 
{
    PriorityQueue<int> pq;
    
    pq.push(10);
    pq.push(30);
    pq.push(20);
    pq.push(5);

    // In a max-heap, the top element is always the largest
    EXPECT_EQ(pq.size(), 4);
    EXPECT_EQ(pq.top(), 30);
}

TEST(PriorityQueueTest, PopSequence) 
{
    PriorityQueue<int> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);

    // They should come out in strictly descending order
    EXPECT_EQ(pq.top(), 30);
    pq.pop();
    
    EXPECT_EQ(pq.top(), 20);
    pq.pop();
    
    EXPECT_EQ(pq.top(), 10);
    pq.pop();

    EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueTest, MinHeapWithGreater) 
{
    // We pass std::greater to make a min-heap
    PriorityQueue<int, std::vector<int>, std::greater<int>> pq;
    
    pq.push(50);
    pq.push(10);
    pq.push(40);

    // Now the smallest element should be at the top
    EXPECT_EQ(pq.top(), 10);
    pq.pop();
    EXPECT_EQ(pq.top(), 40);
}

TEST(PriorityQueueTest, ConstructorFromRange)
{
    mystl::Vector<int> vec = {10, 50, 20, 30, 15};

    PriorityQueue<int> pq(vec.begin(), vec.end());

    EXPECT_EQ(pq.size(), 5);
    EXPECT_EQ(pq.top(), 50);
}

TEST(PriorityQueueTest, AllocatorConstruction)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Vector<int, Alloc>;

    PriorityQueue<int, Container> pq(Alloc(42));

    EXPECT_EQ(pq.get_allocator().id, 42);
    pq.push(10);
    pq.push(30);
    pq.push(20);
    EXPECT_EQ(pq.top(), 30);
}

TEST(PriorityQueueTest, RangeConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Vector<int, Alloc>;

    mystl::Vector<int> values = { 10, 50, 20, 30, 15};
    PriorityQueue<int, Container> pq(values.begin(), values.end(), mystl::less(), Alloc(7));

    EXPECT_EQ(pq.get_allocator().id, 7);
    EXPECT_EQ(pq.size(), 5);
    EXPECT_EQ(pq.top(), 50);
}

TEST(PriorityQueueTest, ContainerConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Vector<int, Alloc>;

    Container values({10, 50, 20}, Alloc(1));
    PriorityQueue<int, Container> pq(mystl::less(), values, Alloc(9));

    EXPECT_EQ(pq.get_allocator().id, 9);
    EXPECT_EQ(pq.size(), 3);
    EXPECT_EQ(pq.top(), 50);
}
