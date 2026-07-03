#include <gtest/gtest.h>

#include "mystl/type_traits.hpp"

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