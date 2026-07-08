#include "mystl/memory.hpp"
#include <gtest/gtest.h>

// Helper class for leak detection
struct Tracker 
{
    static int instance_count;
    int value;

    Tracker(int val = 0) : value(val) { ++instance_count; }
    ~Tracker() { --instance_count; }
};

int Tracker::instance_count = 0;

// Test fixture to ensure the counter is reset before each test
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
    // 1. Check EBCO: unique_ptr with the default deleter
    // should be the same size as a raw pointer.
    static_assert(sizeof(mystl::unique_ptr<int>) == sizeof(int*), "EBCO failed! unique_ptr has memory overhead.");

    // 2. Basic construction and destruction
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        mystl::unique_ptr<Tracker> p(new Tracker(42));
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 42);
        EXPECT_EQ((*p).value, 42);
        EXPECT_NE(p.get(), nullptr);
        EXPECT_TRUE(static_cast<bool>(p));
    }
    // After exiting the scope the object should be destroyed
    EXPECT_EQ(Tracker::instance_count, 0);
}

TEST_F(UniquePtrTest, MoveSemantics) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        mystl::unique_ptr<Tracker> p1(new Tracker(100));
        EXPECT_EQ(Tracker::instance_count, 1);

        // Move constructor
        mystl::unique_ptr<Tracker> p2(mystl::move(p1));
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_FALSE(static_cast<bool>(p1)); // p1 is now empty
        EXPECT_TRUE(static_cast<bool>(p2));  // p2 now owns the resource
        EXPECT_EQ(p2->value, 100);

        // Move assignment operator
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
        
        // Test release()
        Tracker* raw = p.release();
        EXPECT_FALSE(static_cast<bool>(p));
        EXPECT_EQ(Tracker::instance_count, 1);

        // Test reset()
        p.reset(raw); // Return ownership back
        EXPECT_TRUE(static_cast<bool>(p));
        
        p.reset(new Tracker(20)); // Old object should be deleted, new one takes its place
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 20);

        p.reset(); // Full reset
        EXPECT_FALSE(static_cast<bool>(p));
        EXPECT_EQ(Tracker::instance_count, 0);
    }
}

TEST_F(UniquePtrTest, ArraySpecialization) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        // Test specialization for arrays T[]
        mystl::unique_ptr<Tracker[]> arr(new Tracker[3]{Tracker(1), Tracker(2), Tracker(3)});
        EXPECT_EQ(Tracker::instance_count, 3);
        
        EXPECT_EQ(arr[0].value, 1);
        EXPECT_EQ(arr[1].value, 2);
        EXPECT_EQ(arr[2].value, 3);
    }
    // delete[] should correctly call destructors for all elements
    EXPECT_EQ(Tracker::instance_count, 0);
}

TEST_F(UniquePtrTest, MakeUnique) 
{
    EXPECT_EQ(Tracker::instance_count, 0);
    {
        // Single object
        auto p = mystl::make_unique<Tracker>(777);
        EXPECT_EQ(Tracker::instance_count, 1);
        EXPECT_EQ(p->value, 777);
    }
    EXPECT_EQ(Tracker::instance_count, 0);

    {
        // Array via make_unique
        auto arr = mystl::make_unique<Tracker[]>(5);
        EXPECT_EQ(Tracker::instance_count, 5);
    }
    EXPECT_EQ(Tracker::instance_count, 0);
}