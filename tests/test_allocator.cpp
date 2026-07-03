#include <gtest/gtest.h>
#include "mystl/allocator.hpp"
#include "mystl/memory.hpp"
#include "mystl/type_traits.hpp"

#include <cstddef>
#include <new>
#include <string>


// ============================================================================
// STATELESS DEFAULT ALLOCATOR (mystl::Allocator)
// ============================================================================
TEST(AllocatorTraitsTest, StatelessDefaults)
{
    using A  = mystl::Allocator<int>;
    using Tr = mystl::allocator_traits<A>;

    static_assert(mystl::is_same_v<Tr::value_type, int>);
    static_assert(mystl::is_same_v<Tr::pointer, int*>);
    static_assert(mystl::is_same_v<Tr::const_pointer, const int*>);
    static_assert(mystl::is_same_v<Tr::void_pointer, void*>);
    static_assert(mystl::is_same_v<Tr::const_void_pointer, const void*>);

    // Stateless => always equal, nothing propagates by default.
    static_assert(Tr::is_always_equal::value, "stateless allocator is always equal");
    static_assert(!Tr::propagate_on_container_copy_assignment::value);
    static_assert(!Tr::propagate_on_container_move_assignment::value);
    static_assert(!Tr::propagate_on_container_swap::value);
    SUCCEED();
}

TEST(AllocatorTraitsTest, MaxSizeDefault)
{
    using A  = mystl::Allocator<int>;
    using Tr = mystl::allocator_traits<A>;
    A a;
    EXPECT_EQ(Tr::max_size(a), static_cast<std::size_t>(-1) / sizeof(int));
}

TEST(AllocatorTraitsTest, SelectOnCopyDefaultReturnsCopy)
{
    using A  = mystl::Allocator<int>;
    using Tr = mystl::allocator_traits<A>;
    A a;
    A b = Tr::select_on_container_copy_construction(a);
    EXPECT_TRUE(a == b); // stateless allocators compare equal
}

// ============================================================================
// STATEFUL ALLOCATOR THAT DECLARES THE FULL CONTRACT
// ============================================================================
template <typename T>
struct StatefulAlloc
{
    using value_type = T;
    int id = 0;

    using propagate_on_container_copy_assignment = mystl::true_type;
    using propagate_on_container_move_assignment = mystl::true_type;
    using propagate_on_container_swap            = mystl::true_type;
    using is_always_equal                        = mystl::false_type;

    template <typename U> struct rebind { using other = StatefulAlloc<U>; };

    StatefulAlloc() = default;
    explicit StatefulAlloc(int i) : id(i) {}
    template <typename U> StatefulAlloc(const StatefulAlloc<U>& o) : id(o.id) {}

    T* allocate(std::size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }

    std::size_t   max_size() const noexcept { return 42; }
    StatefulAlloc select_on_container_copy_construction() const { return StatefulAlloc(id + 100); }
};

TEST(AllocatorTraitsTest, StatefulDetection)
{
    using A  = StatefulAlloc<int>;
    using Tr = mystl::allocator_traits<A>;

    static_assert(Tr::propagate_on_container_copy_assignment::value);
    static_assert(Tr::propagate_on_container_move_assignment::value);
    static_assert(Tr::propagate_on_container_swap::value);
    static_assert(!Tr::is_always_equal::value, "stateful allocator is not always equal");

    A a(7);
    EXPECT_EQ(Tr::max_size(a), 42u);                                  // allocator-provided max_size
    A b = Tr::select_on_container_copy_construction(a);
    EXPECT_EQ(b.id, 107);                                             // allocator-provided socc hook
}

TEST(AllocatorTraitsTest, StatefulRebindPreservesContract)
{
    using A   = StatefulAlloc<int>;
    using Tr  = mystl::allocator_traits<A>;
    using RB  = Tr::rebind_alloc<double>;
    using RBt = mystl::allocator_traits<RB>;

    static_assert(mystl::is_same_v<RB::value_type, double>);
    static_assert(RBt::propagate_on_container_move_assignment::value);
    static_assert(!RBt::is_always_equal::value);
    SUCCEED();
}

// ============================================================================
// MINIMAL ALLOCATOR: EVERYTHING OPTIONAL OMITTED -> TRAITS SYNTHESIZE DEFAULTS
// ============================================================================
template <typename T>
struct MinimalAlloc
{
    using value_type = T;
    template <typename U> struct rebind { using other = MinimalAlloc<U>; };
    T* allocate(std::size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
    void deallocate(T* p, std::size_t) noexcept { ::operator delete(p); }
};

TEST(AllocatorTraitsTest, MinimalSynthesizedDefaults)
{
    using A  = MinimalAlloc<double>;
    using Tr = mystl::allocator_traits<A>;

    static_assert(mystl::is_same_v<Tr::pointer, double*>);
    static_assert(mystl::is_same_v<Tr::const_pointer, const double*>);
    static_assert(mystl::is_same_v<Tr::size_type, std::size_t>);
    static_assert(mystl::is_same_v<Tr::difference_type, std::ptrdiff_t>);

    static_assert(!Tr::propagate_on_container_move_assignment::value);
    static_assert(Tr::is_always_equal::value, "empty allocator defaults to always-equal");

    A a;
    EXPECT_EQ(Tr::max_size(a), static_cast<std::size_t>(-1) / sizeof(double));

    // No socc hook: default returns a copy (compiles and runs).
    A b = Tr::select_on_container_copy_construction(a);
    (void)b;
    SUCCEED();
}
