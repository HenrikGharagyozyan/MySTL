#include <gtest/gtest.h>
#include "mystl/tuple.hpp"
#include <string>

using namespace mystl;

TEST(TupleTest, ConstructionAndGet)
{
    Tuple<int, double, std::string> t(42, 3.14, "hello");
    
    EXPECT_EQ(mystl::get<0>(t), 42);
    EXPECT_DOUBLE_EQ(mystl::get<1>(t), 3.14);
    EXPECT_EQ(mystl::get<2>(t), "hello");
}

TEST(TupleTest, MakeTupleAndDecay)
{
    int x = 10;
    auto t = mystl::make_tuple(x, 5.5, "world");
    
    // Проверка decay (const char[] -> const char*)
    static_assert(std::is_same_v<tuple_element_t<2, decltype(t)>, const char*>);
    
    EXPECT_EQ(mystl::get<0>(t), 10);
    EXPECT_EQ(mystl::get<1>(t), 5.5);
    EXPECT_STREQ(mystl::get<2>(t), "world");
}

TEST(TupleTest, EqualityOperators)
{
    auto t1 = mystl::make_tuple(1, 2.0, 'c');
    auto t2 = mystl::make_tuple(1, 2.0, 'c');
    auto t3 = mystl::make_tuple(1, 3.0, 'c');

    EXPECT_TRUE(t1 == t2);
    EXPECT_FALSE(t1 == t3);
    EXPECT_TRUE(t1 != t3);
}

TEST(TupleTest, RelationalOperators)
{
    auto t1 = mystl::make_tuple(1, 2, 3);
    auto t2 = mystl::make_tuple(1, 2, 4);
    auto t3 = mystl::make_tuple(2, 0, 0);

    // Сравнение последнего элемента
    EXPECT_TRUE(t1 < t2);
    EXPECT_TRUE(t2 > t1);
    
    // Сравнение первого элемента
    EXPECT_TRUE(t2 < t3);
    
    EXPECT_TRUE(t1 <= t2);
    EXPECT_TRUE(t2 >= t1);
}

TEST(TupleTest, TupleSizeAndElement)
{
    using MyTup = Tuple<int, char, float>;
    
    static_assert(tuple_size_v<MyTup> == 3, "Size should be 3");
    static_assert(std::is_same_v<tuple_element_t<0, MyTup>, int>, "0 is int");
    static_assert(std::is_same_v<tuple_element_t<1, MyTup>, char>, "1 is char");
    static_assert(std::is_same_v<tuple_element_t<2, MyTup>, float>, "2 is float");
}