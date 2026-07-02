#include <gtest/gtest.h>
#include "mystl/vector.hpp"
#include <string>
#include <stdexcept>

TEST(VectorTest, DefaultConstruction)
{
    mystl::Vector<int> vec;
    EXPECT_EQ(vec.size(), 0);
    EXPECT_EQ(vec.capacity(), 0);
    EXPECT_TRUE(vec.empty());
}

TEST(VectorTest, PushBackAndCapacityGrowth)
{
    mystl::Vector<int> vec;
    vec.push_back(1);
    EXPECT_EQ(vec.size(), 1);
    EXPECT_GE(vec.capacity(), 1);

    vec.push_back(2);
    vec.push_back(3);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], 1);
    EXPECT_EQ(vec[2], 3);
}

TEST(VectorTest, ComplexObjects)
{
    mystl::Vector<std::string> vec;
    vec.push_back("Hello");
    vec.push_back("World");

    EXPECT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "Hello");

    vec.pop_back();
    EXPECT_EQ(vec.size(), 1);
}

TEST(VectorTest, RangeBasedForLoop)
{
    mystl::Vector<int> vec = {1, 2, 3, 4, 5};
    int sum = 0;
    for (int val : vec)
    {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST(VectorTest, MoveSemantics)
{
    mystl::Vector<int> vec1 = {1, 2, 3};
    const int *old_data_ptr = &vec1[0];

    // Move vec1 to vec2
    mystl::Vector<int> vec2 = mystl::move(vec1);

    // vec1 should become empty
    EXPECT_EQ(vec1.size(), 0);
    // vec2 should receive the data, without copying (same pointer)
    EXPECT_EQ(vec2.size(), 3);
    EXPECT_EQ(vec2.data(), old_data_ptr); // Use data() to get the underlying pointer
}

TEST(VectorTest, CopySemantics)
{
    mystl::Vector<int> vec1 = {10, 20};
    mystl::Vector<int> vec2 = vec1; // Copy

    EXPECT_EQ(vec2.size(), 2);
    EXPECT_EQ(vec2[0], 10);

    // Modifying the copy should not affect the original
    vec2[0] = 99;
    EXPECT_EQ(vec1[0], 10);
}

TEST(VectorTest, InsertElements)
{
    mystl::Vector<int> vec = {1, 2, 4, 5};

    // Insert 3 at index 2 (before 4)
    auto it = vec.insert(vec.begin() + 2, 3);

    EXPECT_EQ(*it, 3);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[2], 3);
    EXPECT_EQ(vec[4], 5);
}

TEST(VectorTest, EraseElements)
{
    mystl::Vector<int> vec = {10, 20, 30, 40};

    // Erase 20 (index 1)
    auto it = vec.erase(vec.begin() + 1);

    EXPECT_EQ(*it, 30); // Should return an iterator to the next element
    EXPECT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[1], 30);
}

// ============================================================================
// STRONG EXCEPTION GUARANTEE ON REALLOCATION (move_if_noexcept)
// ============================================================================
namespace
{
    // A type whose move constructor is NOT noexcept and whose copy constructor
    // can be armed to throw. Because the move can throw, move_if_noexcept must
    // fall back to copying during reallocation, keeping the source intact.
    struct ThrowOnCopy
    {
        int value;

        static int  live_instances;
        static int  copies_until_throw; // -1 disables throwing

        explicit ThrowOnCopy(int v = 0) : value(v) { ++live_instances; }

        ThrowOnCopy(const ThrowOnCopy& other) : value(other.value)
        {
            if (copies_until_throw == 0)
                throw std::runtime_error("ThrowOnCopy: forced copy failure");
            if (copies_until_throw > 0)
                --copies_until_throw;
            ++live_instances;
        }

        // Deliberately NOT noexcept -> move_if_noexcept will prefer the copy path.
        ThrowOnCopy(ThrowOnCopy&& other) noexcept(false) : value(other.value)
        {
            other.value = -1;
            ++live_instances;
        }

        ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
        ThrowOnCopy& operator=(ThrowOnCopy&&) noexcept(false) = default;

        ~ThrowOnCopy() { --live_instances; }
    };

    int ThrowOnCopy::live_instances    = 0;
    int ThrowOnCopy::copies_until_throw = -1;
}

TEST(VectorTest, MoveIfNoexceptPrefersCopyForThrowingMove)
{
    // A throwing move must NOT be selected by move_if_noexcept.
    static_assert(!mystl::is_nothrow_move_constructible_v<ThrowOnCopy>,
                  "test type must have a throwing move ctor");
    static_assert(mystl::is_copy_constructible_v<ThrowOnCopy>,
                  "test type must be copyable");
}

TEST(VectorTest, ReserveStrongGuaranteeOnThrowingCopy)
{
    ThrowOnCopy::live_instances    = 0;
    ThrowOnCopy::copies_until_throw = -1;

    mystl::Vector<ThrowOnCopy> vec;
    vec.reserve(4);
    vec.emplace_back(1);
    vec.emplace_back(2);
    vec.emplace_back(3);
    vec.emplace_back(4); // fill exactly to capacity so the next append reallocates

    // Snapshot observable state right before the failing reallocation.
    const ThrowOnCopy* old_data = vec.data();
    const auto sz = vec.size();

    // Arm the 2nd copy during the next reallocation to throw.
    ThrowOnCopy::copies_until_throw = 1;

    // A single append now forces reallocation (capacity 4 -> 8), transferring
    // via copy (throwing move); the 2nd element's copy throws mid-transfer.
    bool threw = false;
    try
    {
        vec.emplace_back(99);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    ASSERT_TRUE(threw);

    // Strong guarantee: the vector is unchanged by the failed reallocation.
    EXPECT_EQ(vec.data(), old_data);          // original buffer retained
    EXPECT_EQ(vec.size(), sz);                // size unchanged
    EXPECT_EQ(vec[0].value, 1);               // elements intact (not moved-from)
    EXPECT_EQ(vec[1].value, 2);
    EXPECT_EQ(vec[2].value, 3);
    EXPECT_EQ(vec[3].value, 4);

    ThrowOnCopy::copies_until_throw = -1;
}

TEST(VectorTest, ReallocationLeaksNothingOnThrow)
{
    ThrowOnCopy::live_instances    = 0;
    ThrowOnCopy::copies_until_throw = -1;

    {
        mystl::Vector<ThrowOnCopy> vec;
        vec.reserve(2);
        vec.emplace_back(1);
        vec.emplace_back(2);

        ThrowOnCopy::copies_until_throw = 1; // throw on 2nd copy during realloc

        try
        {
            for (int i = 0; i < 8; ++i)
                vec.emplace_back(i);
        }
        catch (const std::runtime_error&)
        {
        }

        ThrowOnCopy::copies_until_throw = -1;
    }

    // Every constructed element (including the temporary new buffer that was
    // rolled back) must have been destroyed: no leak, no double-free.
    EXPECT_EQ(ThrowOnCopy::live_instances, 0);
}