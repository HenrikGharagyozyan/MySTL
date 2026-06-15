#include <gtest/gtest.h>
#include <vector>
#include <functional>
#include "mystl/priority_queue.hpp"

using namespace mystl;

TEST(PriorityQueueTest, DefaultConstruction) 
{
    PriorityQueue<int> pq;
    EXPECT_TRUE(pq.empty());
    EXPECT_EQ(pq.size(), 0);
}

TEST(PriorityQueueTest, PushAndTopMaxHeap) 
{
    PriorityQueue<int> pq;
    
    pq.push(10);
    pq.push(30);
    pq.push(20);
    pq.push(5);

    // В Max-Heap сверху всегда самый большой элемент
    EXPECT_EQ(pq.size(), 4);
    EXPECT_EQ(pq.top(), 30);
}

TEST(PriorityQueueTest, PopSequence) 
{
    PriorityQueue<int> pq;
    pq.push(10);
    pq.push(30);
    pq.push(20);

    // Должны выходить строго по убыванию
    EXPECT_EQ(pq.top(), 30);
    pq.pop();
    
    EXPECT_EQ(pq.top(), 20);
    pq.pop();
    
    EXPECT_EQ(pq.top(), 10);
    pq.pop();

    EXPECT_TRUE(pq.empty());
}

TEST(PriorityQueueTest, MinHeapWithGreater) 
{
    // Передаем std::greater, чтобы сделать Min-Heap
    PriorityQueue<int, std::vector<int>, std::greater<int>> pq;
    
    pq.push(50);
    pq.push(10);
    pq.push(40);

    // Теперь сверху должен быть самый маленький элемент
    EXPECT_EQ(pq.top(), 10);
    pq.pop();
    EXPECT_EQ(pq.top(), 40);
}

TEST(PriorityQueueTest, ConstructorFromRange) 
{
    std::vector<int> vec = {10, 50, 20, 30, 15};
    
    // Вызовется make_heap внутри конструктора
    PriorityQueue<int> pq(vec.begin(), vec.end());
    
    EXPECT_EQ(pq.size(), 5);
    EXPECT_EQ(pq.top(), 50);
}