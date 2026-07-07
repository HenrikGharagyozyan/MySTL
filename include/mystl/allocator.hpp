#pragma once

#include "cstddef.hpp" // for mystl::size_t, mystl::ptrdiff_t
#include <new>     // for std::bad_alloc

namespace mystl
{
    template <typename T>
    struct Allocator
    {
        using value_type      = T;
        using pointer         = T*;
        using const_pointer   = const T*;
        using reference       = T&;
        using const_reference = const T&;
        using size_type       = mystl::size_t;
        using difference_type = mystl::ptrdiff_t;

        // ========================================================================
        // REBIND MECHANISM
        // Allows the container to change the type of allocated memory (e.g., from T to Node)
        // ========================================================================
        template <typename U>
        struct rebind
        {
            using other = Allocator<U>;
        };

        // ========================================================================
        // CONSTRUCTORS
        // ========================================================================
        constexpr Allocator() noexcept = default;
        constexpr Allocator(const Allocator&) noexcept = default;

        template <typename U>
        constexpr Allocator(const Allocator<U>&) noexcept {}

        // ========================================================================
        // RAW MEMORY MANAGEMENT
        // ========================================================================
        
        [[nodiscard]] 
        pointer allocate(size_type n)
        {
            // Protection from overflow
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

        void deallocate(pointer p, size_type /*n*/) noexcept
        {
            // In the basic allocator, the size doesn't matter when deleting (::operator delete will figure it out)
            ::operator delete(p);
        }

    };

    // Stateless allocators are always equal to each other
    template <typename T, typename U>
    constexpr bool operator==(const Allocator<T>&, const Allocator<U>&) noexcept 
    { 
        return true; 
    }

    template <typename T, typename U>
    constexpr bool operator!=(const Allocator<T>&, const Allocator<U>&) noexcept 
    { 
        return false; 
    }

} // namespace mystl