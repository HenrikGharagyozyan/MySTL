#include <gtest/gtest.h>
#include "mystl/pair.hpp"
#include <string>
#include <type_traits>

using namespace mystl;

// ============================================================================
// 1. Конструкторы
// ============================================================================

TEST(PairTest, DefaultConstructor)
{
    Pair<int, double> p;
    EXPECT_EQ(p.first, 0);
    EXPECT_EQ(p.second, 0.0);
}

TEST(PairTest, ValueConstructor)
{
    Pair<int, std::string> p(42, "hello");
    EXPECT_EQ(p.first, 42);
    EXPECT_EQ(p.second, "hello");
}

TEST(PairTest, CopyAndMoveConstructor)
{
    Pair<int, int> p1(1, 2);
    
    // Копирование
    Pair<int, int> p2(p1);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);

    // Перемещение
    Pair<int, int> p3(mystl::move(p1));
    EXPECT_EQ(p3.first, 1);
    EXPECT_EQ(p3.second, 2);
}

// ============================================================================
// 2. make_pair и perfect forwarding (decay_t)
// ============================================================================

TEST(PairTest, MakePair)
{
    auto p = mystl::make_pair(10, 3.14);
    
    // Проверяем типы, чтобы убедиться, что decay_t работает правильно
    static_assert(std::is_same_v<decltype(p.first), int>, "first should be int");
    static_assert(std::is_same_v<decltype(p.second), double>, "second should be double");
    
    EXPECT_EQ(p.first, 10);
    EXPECT_EQ(p.second, 3.14);

    // Проверка decay для массивов (const char[N] -> const char*)
    int x = 5;
    auto p2 = mystl::make_pair(x, "test_string");
    static_assert(std::is_same_v<decltype(p2.second), const char*>, "string literal should decay to const char*");
    
    EXPECT_EQ(p2.first, 5);
    EXPECT_STREQ(p2.second, "test_string");
}

// ============================================================================
// 3. Операторы сравнения (Лексикографическое)
// ============================================================================

TEST(PairTest, Comparisons)
{
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(1, 2);
    Pair<int, int> p3(1, 3);
    Pair<int, int> p4(0, 5);

    // Равенство
    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 != p2);
    
    // Меньше/Больше (первый элемент совпадает, сравниваем второй)
    EXPECT_TRUE(p1 != p3);
    EXPECT_TRUE(p1 < p3);
    EXPECT_TRUE(p3 > p1);
    EXPECT_TRUE(p1 <= p3);
    EXPECT_TRUE(p3 >= p1);
    
    // Меньше/Больше (отличается первый элемент)
    EXPECT_TRUE(p4 < p1); // 0 < 1
    EXPECT_TRUE(p1 > p4);
    EXPECT_TRUE(p4 <= p1);
}

// ============================================================================
// 4. Swap
// ============================================================================

TEST(PairTest, Swap)
{
    Pair<int, int> p1(1, 2);
    Pair<int, int> p2(3, 4);
    
    // Метод класса
    p1.swap(p2);
    EXPECT_EQ(p1.first, 3);
    EXPECT_EQ(p1.second, 4);
    EXPECT_EQ(p2.first, 1);
    EXPECT_EQ(p2.second, 2);
    
    // Свободная функция (ADL)
    mystl::swap(p1, p2);
    EXPECT_EQ(p1.first, 1);
    EXPECT_EQ(p1.second, 2);
    EXPECT_EQ(p2.first, 3);
    EXPECT_EQ(p2.second, 4);
}