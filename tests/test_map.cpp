#include <gtest/gtest.h>
#include <string>
#include "mystl/map.hpp"

using namespace mystl;

TEST(MapTest, DefaultConstructionAndEmpty) 
{
    Map<std::string, int> m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0);
}

TEST(MapTest, OperatorBracketAccess) 
{
    Map<std::string, int> m;
    
    // Вставка через оператор []
    m["Health"] = 100;
    m["Mana"] = 50;
    
    EXPECT_EQ(m.size(), 2);
    EXPECT_EQ(m["Health"], 100);
    EXPECT_EQ(m["Mana"], 50);

    // Доступ к несуществующему ключу создает его с дефолтным значением (0)
    EXPECT_EQ(m["Stamina"], 0);
    EXPECT_EQ(m.size(), 3);
}

TEST(MapTest, AtThrowsOnMissing) 
{
    Map<int, std::string> m;
    m.insert({1, "Player"});
    
    EXPECT_EQ(m.at(1), "Player");
    EXPECT_THROW(m.at(99), std::out_of_range);
}

TEST(MapTest, InsertOrAssignAndTryEmplace) 
{
    Map<int, std::string> m;
    
    // Вставка
    auto res1 = m.insert_or_assign(1, "Enemy");
    EXPECT_TRUE(res1.second);
    
    // Перезапись
    auto res2 = m.insert_or_assign(1, "Boss");
    EXPECT_FALSE(res2.second);
    EXPECT_EQ(m[1], "Boss");

    // try_emplace (не должен перезаписывать, если ключ уже есть)
    auto res3 = m.try_emplace(1, "Minion");
    EXPECT_FALSE(res3.second);
    EXPECT_EQ(m[1], "Boss"); // Значение осталось прежним
}

TEST(MapTest, FindAndContains) 
{
    Map<int, double> m = {{1, 1.1}, {2, 2.2}, {3, 3.3}};
    
    EXPECT_TRUE(m.contains(2));
    EXPECT_FALSE(m.contains(4));

    auto it = m.find(3);
    ASSERT_NE(it, m.end());
    EXPECT_EQ(it->second, 3.3);
    
    // Проверка мутабельности значения через итератор
    it->second = 9.9;
    EXPECT_EQ(m[3], 9.9);
}

TEST(MapTest, Erase) 
{
    Map<std::string, int> m = {{"A", 1}, {"B", 2}, {"C", 3}};
    
    EXPECT_EQ(m.erase("B"), 1);
    EXPECT_EQ(m.size(), 2);
    EXPECT_FALSE(m.contains("B"));
    EXPECT_EQ(m.erase("Z"), 0); // Удаление несуществующего
}