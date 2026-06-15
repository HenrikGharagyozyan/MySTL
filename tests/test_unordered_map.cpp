#include <gtest/gtest.h>
#include <string>
#include "mystl/unordered_map.hpp"

using namespace mystl;

TEST(UnorderedMapTest, DefaultConstructor) 
{
    UnorderedMap<int, std::string> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);
}

TEST(UnorderedMapTest, InsertAndFind) 
{
    UnorderedMap<int, std::string> map;
    auto res1 = map.insert({1, "HydraEngine"});
    auto res2 = map.insert({2, "OpenGL"});

    EXPECT_TRUE(res1.second);
    EXPECT_EQ(map.size(), 2);

    auto it = map.find(1);
    ASSERT_NE(it, map.end());
    EXPECT_EQ(it->second, "HydraEngine");

    // Повторный insert того же ключа должен провалиться
    auto res3 = map.insert({1, "Fake"});
    EXPECT_FALSE(res3.second);
    EXPECT_EQ(map.size(), 2);
}

TEST(UnorderedMapTest, BracketOperator) 
{
    UnorderedMap<std::string, int> map;
    map["RenderThreads"] = 4;
    map["WindowWidth"] = 1920;

    EXPECT_EQ(map["RenderThreads"], 4);
    EXPECT_EQ(map.at("WindowWidth"), 1920);

    // Доступ по несуществующему ключу через [] должен создать дефолтный элемент
    EXPECT_EQ(map["FPS"], 0); 
    EXPECT_EQ(map.size(), 3);
}

TEST(UnorderedMapTest, EraseElements) 
{
    UnorderedMap<int, char> map;
    map[10] = 'a';
    map[20] = 'b';
    map[30] = 'c';

    EXPECT_EQ(map.erase(20), 1);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.find(20), map.end());

    // Удаление несуществующего ключа
    EXPECT_EQ(map.erase(99), 0);
}

TEST(UnorderedMapTest, MoveSemantics) 
{
    UnorderedMap<int, std::string> map1;
    map1[1] = "Data";

    UnorderedMap<int, std::string> map2 = mystl::move(map1);

    EXPECT_TRUE(map1.empty());
    EXPECT_EQ(map1.bucket_count(), 0);
    
    EXPECT_EQ(map2.size(), 1);
    EXPECT_EQ(map2[1], "Data");
}

TEST(UnorderedMapTest, IteratorIteration) 
{
    UnorderedMap<int, int> map;
    map[1] = 10;
    map[2] = 20;
    map[3] = 30;

    int sum_keys = 0;
    int sum_vals = 0;

    for (auto& pair : map) 
    {
        sum_keys += pair.first;
        sum_vals += pair.second;
    }

    EXPECT_EQ(sum_keys, 6);
    EXPECT_EQ(sum_vals, 60);
}