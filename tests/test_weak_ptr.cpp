#include <gtest/gtest.h>
#include "mystl/shared_ptr.hpp"
#include "mystl/weak_ptr.hpp"

static int weak_instance_count = 0;

struct WeakTestObject 
{
    int value;

    WeakTestObject(int v) 
        : value(v) 
    { 
        ++weak_instance_count; 
    }

    ~WeakTestObject() 
    { 
        --weak_instance_count; 
    }
};

class WeakPtrTest : public ::testing::Test 
{
protected:
    void SetUp() override { weak_instance_count = 0; }

    void TearDown() override 
    { 
        EXPECT_EQ(weak_instance_count, 0) << "Memory leak detected in weak_ptr test!"; 
    }
};

TEST_F(WeakPtrTest, DefaultConstructor) 
{
    mystl::weak_ptr<WeakTestObject> wp;
    EXPECT_EQ(wp.use_count(), 0);
    EXPECT_TRUE(wp.expired());
    
    auto sp = wp.lock(); // Пытаемся получить shared_ptr
    EXPECT_FALSE(sp);    // Должен быть пустым
}

TEST_F(WeakPtrTest, ConstructFromShared) 
{
    auto sp = mystl::make_shared<WeakTestObject>(42);
    mystl::weak_ptr<WeakTestObject> wp(sp);
    
    EXPECT_EQ(wp.use_count(), 1); // weak_ptr видит одного владельца
    EXPECT_FALSE(wp.expired());
    
    auto sp2 = wp.lock(); // Успешно получаем временного владельца
    EXPECT_TRUE(sp2);
    EXPECT_EQ(sp2->value, 42);
    EXPECT_EQ(sp.use_count(), 2); // sp + sp2
}

TEST_F(WeakPtrTest, Expiration) 
{
    mystl::weak_ptr<WeakTestObject> wp;
    {
        auto sp = mystl::make_shared<WeakTestObject>(100);
        wp = sp;
        EXPECT_FALSE(wp.expired());
        EXPECT_EQ(wp.use_count(), 1);
    } // sp уничтожается здесь, объект удаляется, но управляющий блок еще жив

    EXPECT_TRUE(wp.expired());
    EXPECT_EQ(wp.use_count(), 0);
    
    auto sp2 = wp.lock(); // Попытка блокировки мертвого объекта
    EXPECT_FALSE(sp2);    // Должен вернуть пустой указатель
}

// === Тест на предотвращение циклических ссылок ===
struct Node 
{
    mystl::shared_ptr<Node> next; // Сильная ссылка вперед
    mystl::weak_ptr<Node> prev;   // Слабая ссылка назад (предотвращает цикл!)
    
    Node() { ++weak_instance_count; }
    ~Node() { --weak_instance_count; }
};

TEST_F(WeakPtrTest, CyclicReferencePrevention) 
{
    {
        auto node1 = mystl::make_shared<Node>();
        auto node2 = mystl::make_shared<Node>();
        
        node1->next = node2;
        node2->prev = node1; // Если бы тут был shared_ptr, случилась бы утечка
        
        EXPECT_EQ(node1.use_count(), 1); 
        EXPECT_EQ(node2.use_count(), 2); // Владеет локальная переменная + node1.next
    }
    // Здесь оба узла должны корректно уничтожиться. 
    // TearDown проверит, что weak_instance_count == 0.
}