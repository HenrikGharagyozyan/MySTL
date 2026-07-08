#include <gtest/gtest.h>
#include "mystl/shared_ptr.hpp"
#include "mystl/utility.hpp"

static int shared_instance_count = 0;

struct SharedTestObject 
{
    int value;

    SharedTestObject(int v) 
        : value(v) 
    { 
        ++shared_instance_count; 
    }

    ~SharedTestObject() 
    { 
        --shared_instance_count; 
    }
};

// Фикстура для тестов (setup/teardown)
class SharedPtrTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        shared_instance_count = 0;
    }

    void TearDown() override 
    {
        // Проверяем, что после каждого теста не осталось висящих объектов
        EXPECT_EQ(shared_instance_count, 0) << "Memory leak detected!";
    }
};

TEST_F(SharedPtrTest, DefaultConstructor) 
{
    mystl::shared_ptr<int> p;
    EXPECT_EQ(p.use_count(), 0);
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_FALSE(p);
}

TEST_F(SharedPtrTest, MakeShared) 
{
    {
        auto p = mystl::make_shared<SharedTestObject>(42);
        EXPECT_EQ(p.use_count(), 1);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ(shared_instance_count, 1);
        EXPECT_TRUE(p.unique());
    }
    // После выхода из блока объект должен быть уничтожен
    EXPECT_EQ(shared_instance_count, 0);
}

TEST_F(SharedPtrTest, CopySemantics) 
{
    auto p1 = mystl::make_shared<SharedTestObject>(10);
    EXPECT_EQ(p1.use_count(), 1);
    {
        auto p2 = p1; // Копирование
        EXPECT_EQ(p1.use_count(), 2);
        EXPECT_EQ(p2.use_count(), 2);
        EXPECT_EQ(p1.get(), p2.get());
    } // p2 уничтожается
    EXPECT_EQ(p1.use_count(), 1);
    EXPECT_EQ(shared_instance_count, 1);
}

TEST_F(SharedPtrTest, MoveSemantics) 
{
    auto p1 = mystl::make_shared<SharedTestObject>(20);
    auto p2 = mystl::move(p1); // Перемещение
    
    EXPECT_EQ(p1.use_count(), 0);
    EXPECT_EQ(p1.get(), nullptr);
    
    EXPECT_EQ(p2.use_count(), 1);
    EXPECT_EQ(p2->value, 20);
    EXPECT_EQ(shared_instance_count, 1);
}

TEST_F(SharedPtrTest, Reset) 
{
    auto p = mystl::make_shared<SharedTestObject>(100);
    EXPECT_EQ(shared_instance_count, 1);
    
    p.reset();
    EXPECT_EQ(p.use_count(), 0);
    EXPECT_EQ(p.get(), nullptr);
    EXPECT_EQ(shared_instance_count, 0);
}