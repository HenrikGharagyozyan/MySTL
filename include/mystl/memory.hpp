#pragma once

#include "utility.hpp"
#include "iterator.hpp"

#include <cstddef>
#include <new>


namespace mystl 
{

    // ========================================================================
    // ADDRESSOF & LIFECYCLE MANAGEMENT
    // ========================================================================
    
    // Safe getting of object address (bypasses overloaded operator&)
    // ========================================================================
    // ADDRESSOF & LIFECYCLE MANAGEMENT
    // ========================================================================

    // Safely obtain an object's address (bypasses an overloaded operator&)
    template <typename T>
    constexpr T* addressof(T& arg) noexcept 
    {
        return __builtin_addressof(arg);
    }

    template <typename T>
    const T* addressof(const T&&) = delete;

    template <typename T, typename... Args>
    constexpr T* construct_at(T* p, Args&&... args) 
    {
        return ::new (const_cast<void*>(static_cast<const volatile void*>(p))) T(mystl::forward<Args>(args)...);
    }

    template <typename T>
    constexpr void destroy_at(T* p) 
    {
        if constexpr (!mystl::is_trivially_destructible_v<T>) 
        {
            p->~T();
        }
    }

    template <typename ForwardIt>
    constexpr void destroy(ForwardIt first, ForwardIt last) 
    {
        for (; first != last; ++first) 
        {
            mystl::destroy_at(mystl::addressof(*first));
        }
    }

    // ========================================================================
    // UNINITIALIZED MEMORY ALGORITHMS
    // ========================================================================
    namespace detail 
    {
        // RAII guard: if construction throws, it will destroy what was already created
        template <typename T>
        struct destroy_guard 
        {
            T* first;
            T* current;

            constexpr ~destroy_guard() 
            {
                if (first != current) 
                {
                    mystl::destroy(first, current);
                }
            }
            
            constexpr void release() noexcept 
            {
                first = current; // Cancel destruction (everything completed successfully)
            }
        };
    }

    template <typename ForwardIt, typename T>
    void uninitialized_fill(ForwardIt first, ForwardIt last, const T& value) 
    {
        using ValueType = typename mystl::iterator_traits<ForwardIt>::value_type;
        detail::destroy_guard<ValueType> guard{mystl::addressof(*first), mystl::addressof(*first)};
        
        for (; first != last; ++first) 
        {
            mystl::construct_at(mystl::addressof(*first), value);
            ++guard.current;
        }
        guard.release();
    }

    template <typename InputIt, typename ForwardIt>
    ForwardIt uninitialized_copy(InputIt first, InputIt last, ForwardIt d_first) 
    {
        using ValueType = typename mystl::iterator_traits<ForwardIt>::value_type;
        detail::destroy_guard<ValueType> guard{mystl::addressof(*d_first), mystl::addressof(*d_first)};
        
        for (; first != last; ++first, (void)++d_first) 
        {
            mystl::construct_at(mystl::addressof(*d_first), *first);
            ++guard.current;
        }
        guard.release();
        return d_first;
    }

    template <typename InputIt, typename ForwardIt>
    ForwardIt uninitialized_move(InputIt first, InputIt last, ForwardIt d_first) 
    {
        using ValueType = typename mystl::iterator_traits<ForwardIt>::value_type;
        detail::destroy_guard<ValueType> guard{mystl::addressof(*d_first), mystl::addressof(*d_first)};
        
        for (; first != last; ++first, (void)++d_first) 
        {
            mystl::construct_at(mystl::addressof(*d_first), mystl::move(*first));
            ++guard.current;
        }
        guard.release();
        return d_first;
    }

    namespace detail 
    {
        template <typename... Ts> using void_t = void;

        // Fallback: if rebind is not found, extract the template arguments and replace the first one
        template <typename Alloc, typename U>
        struct rebind_fallback;

        template <template <typename, typename...> class AllocTemplate, typename T, typename... Args, typename U>
        struct rebind_fallback<AllocTemplate<T, Args...>, U>
        {
            using type = AllocTemplate<U, Args...>;
        };

        template <typename Alloc, typename U, typename = void>
        struct rebind_alloc_helper : rebind_fallback<Alloc, U> {};

        // SFINAE magic: if Alloc::rebind<U>::other exists, this specialization will be selected
        template <typename Alloc, typename U>
        struct rebind_alloc_helper<Alloc, U, void_t<typename Alloc::template rebind<U>::other>> 
        {
            using type = typename Alloc::template rebind<U>::other;
        };
    }

    // ========================================================================
    // ALLOCATOR TRAITS
    // ========================================================================
    template <typename Alloc>
    struct allocator_traits 
    {
        using allocator_type   = Alloc;
        using value_type       = typename Alloc::value_type;
        using pointer          = value_type*;
        using const_pointer    = const value_type*;
        using difference_type  = std::ptrdiff_t;
        using size_type        = std::size_t;

        template <typename T>
        using rebind_alloc = typename detail::rebind_alloc_helper<Alloc, T>::type;

        template <typename T>
        using rebind_traits = allocator_traits<rebind_alloc<T>>;

        [[nodiscard]] static pointer allocate(Alloc& a, size_type n) 
        {
            return a.allocate(n);
        }

        static void deallocate(Alloc& a, pointer p, size_type n) 
        {
            a.deallocate(p, n);
        }

        template <typename T, typename... Args>
        static void construct(Alloc&, T* p, Args&&... args) 
        {
            mystl::construct_at(p, mystl::forward<Args>(args)...);
        }

        template <typename T>
        static void destroy(Alloc&, T* p) 
        {
            mystl::destroy_at(p);
        }

        static Alloc select_on_container_copy_construction(const Alloc& rhs) 
        {
            return rhs; 
        }
    };

} // namespace mystl