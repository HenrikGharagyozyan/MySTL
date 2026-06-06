#include <gtest/gtest.h>
#include "mystl/allocator.hpp"
#include <string>

TEST(AllocatorTest, AllocateAndDeallocateRawMemory)
{
    mystl::Allocator<int> alloc;
    int *ptr = alloc.allocate(10);

    ASSERT_NE(ptr, nullptr); // Ensure that memory was allocated

    alloc.deallocate(ptr, 10);
}

TEST(AllocatorTest, ConstructAndDestroyObjects)
{
    mystl::Allocator<std::string> alloc;

    // Allocate memory for 1 string, but DO NOT call the constructor
    std::string *ptr = alloc.allocate(1);

    // Construct a string at this address, passing arguments (5 'a' characters)
    alloc.construct(ptr, 5, 'a');

    EXPECT_EQ(*ptr, "aaaaa");
    EXPECT_EQ(ptr->length(), 5);

    // Explicitly call the destructor
    alloc.destroy(ptr);

    // Deallocate raw memory
    alloc.deallocate(ptr, 1);
}