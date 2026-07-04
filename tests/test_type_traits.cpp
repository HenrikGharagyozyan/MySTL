#include <gtest/gtest.h>
#include <string>

#include "mystl/type_traits.hpp"
#include "mystl/iterator.hpp"
#include "mystl/list.hpp"

// ============================================================================
// TYPE TRAIT: is_trivially_destructible_v (gates destroy_at's ~T() elision)
// ============================================================================
namespace
{
    struct TrivialDtor    { int x; double y; };
    struct NonTrivialDtor { ~NonTrivialDtor() {} };
}

TEST(TypeTraitsTest, IsTriviallyDestructible)
{
    static_assert(mystl::is_trivially_destructible_v<int>);
    static_assert(mystl::is_trivially_destructible_v<double>);
    static_assert(mystl::is_trivially_destructible_v<int*>);
    static_assert(mystl::is_trivially_destructible_v<TrivialDtor>);

    static_assert(!mystl::is_trivially_destructible_v<NonTrivialDtor>);
    static_assert(!mystl::is_trivially_destructible_v<std::string>);
    SUCCEED();
}
// ============================================================================
// iterator_traits is SFINAE-friendly for non-iterator types (C++17 behavior)
// ============================================================================
namespace
{
    template <typename T, typename = void>
    struct has_it_value_type : mystl::false_type {};
    template <typename T>
    struct has_it_value_type<T, mystl::void_t<typename mystl::iterator_traits<T>::value_type>>
        : mystl::true_type {};
}

TEST(IteratorTraitsTest, SfinaeFriendly)
{
    // Pointers and real class iterators expose the member typedefs.
    static_assert(has_it_value_type<int*>::value);
    static_assert(has_it_value_type<const int*>::value);
    static_assert(has_it_value_type<mystl::List<int>::iterator>::value);
    static_assert(mystl::is_same_v<mystl::iterator_traits<int*>::value_type, int>);

    // Non-iterator types make iterator_traits an EMPTY struct instead of a hard
    // error (this detection would fail to compile before the SFINAE guard).
    static_assert(!has_it_value_type<int>::value);
    static_assert(!has_it_value_type<double>::value);
    struct NotAnIterator {};
    static_assert(!has_it_value_type<NotAnIterator>::value);
    SUCCEED();
}
