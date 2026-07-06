#include "mystl/memory.hpp"
#include <gtest/gtest.h>

// Вспомогательный класс для проверки утечек памяти
struct Tracker 
{
    static int instance_count;
    int value;

    Tracker(int val = 0) : value(val) { ++instance_count; }
    ~Tracker() { --instance_count; }
};

int Tracker::instance_count = 0;

// Тестовый фикстур для гарантии сброса счетчика перед каждым тестом
class UniquePtrTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        Tracker::instance_count = 0;
    }
};

TEST_F(UniquePtrTest, BasicAndEBCO) 
{
    // 1. Проверяем EBCO: размер unique_ptr с дефолтным делейтером 
    // должен быть равен размеру обычного сырого указателя.
    static_assert(sizeof(mystl::unique_ptr<int>) == sizeof(int*), "EBCO failed! unique_ptr has memory overhead.");

    // 2. Базовое создание и деструкция
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        mystl::unique_ptr<Tracker> p(new Tracker(42));
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ((*p).value, 42);
        EXPECT_NE(p.get(), nullptr);
        EXPECT_TRUE(static_cast<bool>(p));
    }
    // После выхода из скоупа объект должен уничтожиться
    EXPECT_EQ(Tracker::instance_count, 0);
}

TEST_F(UniquePtrTest, MoveSemantics) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        mystl::unique_ptr<Tracker> p1(new Tracker(100));
        EXPECT_EQ(Tracker::instance_count, 1);

        // Конструктор перемещения
        mystl::unique_ptr<Tracker> p2(mystl::move(p1));
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_FALSE(static_cast<bool>(p1)); // p1 теперь пустой
        EXPECT_TRUE(static_cast<bool>(p2));  // p2 теперь владеет ресурсом
        EXPECT_EQ(p2->value, 100);

        // Оператор перемещающего присваивания
        mystl::unique_ptr<Tracker> p3;
        p3 = mystl::move(p2);
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_FALSE(static_cast<bool>(p2));
        EXPECT_EQ(p3->value, 100);
    }
    EXPECT_EQ(Tracker::instance_count, 0);
}

TEST_F(UniquePtrTest, Modifiers) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        mystl::unique_ptr<Tracker> p(new Tracker(10));
        
        // Проверка release()
        Tracker* raw = p.release();
        EXPECT_FALSE(static_cast<bool>(p));
        EXPECT_EQ(Tracker::instance_count, 1);

        // Проверка reset()
        p.reset(raw); // Снова отдаем под контроль
        EXPECT_TRUE(static_cast<bool>(p));
        
        p.reset(new Tracker(20)); // Старый должен удалиться, новый встать на его место
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 20);

        p.reset(); // Полный сброс
        EXPECT_FALSE(static_cast<bool>(p));
        EXPECT_EQ(Tracker::instance_count, 0);
    }
}

TEST_F(UniquePtrTest, ArraySpecialization) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        // Проверяем работу специализации для массивов T[]
        mystl::unique_ptr<Tracker[]> arr(new Tracker[3]{Tracker(1), Tracker(2), Tracker(3)});
        EXPECT_EQ(Tracker::instance_count, 3);
        
        EXPECT_EQ(arr[0].value, 1);
        EXPECT_EQ(arr[1].value, 2);
        EXPECT_EQ(arr[2].value, 3);
    }
    // delete[] должен корректно вызвать деструкторы у всех элементов
    EXPECT_EQ(Tracker::instance_count, 0);
}

TEST_F(UniquePtrTest, MakeUnique) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        // Одиночный объект
        auto p = mystl::make_unique<Tracker>(777);
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 777);
    }
    EXPECT_EQ(Tracker::instance_count, 0);

    {
        // Массив через make_unique
        auto arr = mystl::make_unique<Tracker[]>(5);
        EXPECT_EQ(Tracker::instance_count, 5);
    }
    EXPECT_EQ(Tracker::instance_count, 0);
}