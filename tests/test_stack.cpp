#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <new>
#include "mystl/stack.hpp"
#include "mystl/string.hpp"

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
                  mystl::Stack<int, mystl::Deque<int, StatefulAllocator<int>>>,
                  StatefulAllocator<int>>::value,
              "Stack must participate in uses_allocator");

TEST(StackTest, DefaultConstruction) 
{
    mystl::Stack<int> s;
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0);
}

TEST(StackTest, PushPopTop) 
{
    mystl::Stack<int> s;
    s.push(10);
    s.push(20);
    s.push(30);

    // The order should be: 30 (top), 20, 10
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 30);

    s.pop();
    EXPECT_EQ(s.top(), 20);
    EXPECT_EQ(s.size(), 2);
    
    s.pop();
    EXPECT_EQ(s.top(), 10);
    EXPECT_EQ(s.size(), 1);
}

TEST(StackTest, EmplaceAndMove) 
{
    mystl::Stack<mystl::String> s;
    s.emplace("Hydra Engine"); // Uses emplace_back from Deque
    
    EXPECT_EQ(s.top(), "Hydra Engine");

    mystl::Stack<mystl::String> s2 = std::move(s);
    EXPECT_EQ(s2.top(), "Hydra Engine");
    EXPECT_EQ(s2.size(), 1);
    EXPECT_TRUE(s.empty());
}

TEST(StackTest, Swap) 
{
    mystl::Stack<int> s1;
    s1.push(1);
    s1.push(2);

    mystl::Stack<int> s2;
    s2.push(100);

    s1.swap(s2);

    EXPECT_EQ(s1.size(), 1);
    EXPECT_EQ(s1.top(), 100);

    EXPECT_EQ(s2.size(), 2);
    EXPECT_EQ(s2.top(), 2);
}

TEST(StackTest, AllocatorConstruction)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Deque<int, Alloc>;

    mystl::Stack<int, Container> s(Alloc(21));
    EXPECT_EQ(s.get_allocator().id, 21);

    s.push(10);
    s.push(20);
    EXPECT_EQ(s.top(), 20);
}

TEST(StackTest, ContainerAndCopyConstructionWithAllocator)
{
    using Alloc = StatefulAllocator<int>;
    using Container = mystl::Deque<int, Alloc>;

    Container values({1, 2, 3}, Alloc(1));
    mystl::Stack<int, Container> s(values, Alloc(22));

    EXPECT_EQ(s.get_allocator().id, 22);
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s.top(), 3);

    mystl::Stack<int, Container> copy(s, Alloc(23));
    EXPECT_EQ(copy.get_allocator().id, 23);
    EXPECT_EQ(copy.top(), 3);
}
