#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <new>
#include <string>

#include "mystl/queue.hpp"

using namespace mystl;

template <typename T>
struct StatefulAllocator
{
    using value_type = T;
    using pointer = T*;
    using size_type = std::size_t;

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
                  Queue<int, mystl::Deque<int, StatefulAllocator<int>>>,
                  StatefulAllocator<int>>::value,
              "Queue must participate in uses_allocator");

TEST(QueueTest, DefaultConstruction) 
{
    Queue<int> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size(), 0);
}

TEST(QueueTest, PushAndPop) 
{
    Queue<int> q;
    q.push(10);
    q.push(20);
    q.push(30);

    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 30);

    q.pop();
    EXPECT_EQ(q.front(), 20);
    EXPECT_EQ(q.size(), 2);
}

TEST(QueueTest, EmplaceAndMove) 
{
    Queue<std::string> q;
    q.emplace("Hydra"); 
    
    std::string word = "Engine";
    q.push(std::move(word)); // for test

    EXPECT_EQ(q.front(), "Hydra");
    EXPECT_EQ(q.back(), "Engine");
    EXPECT_EQ(q.size(), 2);
}

TEST(QueueTest, Swap) 
{
    Queue<int> q1;
    q1.push(1);
    q1.push(2);

    Queue<int> q2;
    q2.push(99);

    q1.swap(q2);

    EXPECT_EQ(q1.size(), 1);
    EXPECT_EQ(q1.front(), 99);

    EXPECT_EQ(q2.size(), 2);
    EXPECT_EQ(q2.front(), 1);
}

TEST(QueueTest, AllocatorConstruction)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Deque<int, Alloc>;

    Queue<int, Container> q(Alloc(31));
    EXPECT_EQ(q.get_allocator().id, 31);

    q.push(10);
    q.push(20);
    EXPECT_EQ(q.front(), 10);
    EXPECT_EQ(q.back(), 20);
}

TEST(QueueTest, ContainerAndCopyConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Deque<int, Alloc>;

    Container values({1, 2, 3}, Alloc(1));
    Queue<int, Container> q(values, Alloc(32));

    EXPECT_EQ(q.get_allocator().id, 32);
    EXPECT_EQ(q.size(), 3);
    EXPECT_EQ(q.front(), 1);
    EXPECT_EQ(q.back(), 3);

    Queue<int, Container> copy(q, Alloc(33));
    EXPECT_EQ(copy.get_allocator().id, 33);
    EXPECT_EQ(copy.front(), 1);
    EXPECT_EQ(copy.back(), 3);
}
