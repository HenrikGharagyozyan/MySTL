#pragma once

#include <cstddef>     // for std::size_t
#include <new>         // for placement new and std::bad_alloc
#include "utility.hpp" // for mystl::forward

namespace mystl
{

    template <typename T>
    struct Allocator
    {
        using value_type = T;
        using pointer = T *;
        using size_type = std::size_t;

        // Standard constructors
        constexpr Allocator() noexcept = default;
        constexpr Allocator(const Allocator&) noexcept = default;

        template <typename U>
        constexpr Allocator(const Allocator<U>&) noexcept {}

        // 1. Raw memory allocation (without invoking constructors)
        [[nodiscard]] pointer allocate(size_type n)
        {
            if (n > static_cast<size_type>(-1) / sizeof(T))
            {
                throw std::bad_alloc();
            }
            if (auto p = static_cast<pointer>(::operator new(n * sizeof(T))))
            {
                return p;
            }
            throw std::bad_alloc();
        }

        // 2. Memory deallocation (without invoking destructors)
        void deallocate(pointer p, size_type n) noexcept
        {
            (void)n; // In the standard base allocator, the size is ignored during deletion
            ::operator delete(p);
        }

        // 3. Constructing an object at a specific address (Placement New)
        template <typename U, typename... Args>
        void construct(U* p, Args&&... args)
        {
            // Invoke the constructor of U at address p, perfectly forwarding arguments
            ::new (static_cast<void*>(p)) U(mystl::forward<Args>(args)...);
        }

        // 4. Destroying an object (explicit destructor call)
        template <typename U>
        void destroy(U* p)
        {
            p->~U();
        }
    };

    // Stateless allocators are always equal to each other
    template <typename T, typename U>
    constexpr bool operator==(const Allocator<T>&, const Allocator<U>&) noexcept { return true; }

    template <typename T, typename U>
    constexpr bool operator!=(const Allocator<T>&, const Allocator<U>&) noexcept { return false; }

} // namespace mystl