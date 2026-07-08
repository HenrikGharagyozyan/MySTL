#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "type_traits.hpp" // mystl::void_t

#include <cstddef>
#include <new>

namespace mystl 
{
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

    template <typename InputIt, typename ForwardIt>
    ForwardIt uninitialized_move_if_noexcept(InputIt first, InputIt last, ForwardIt d_first)
    {
        using ValueType = typename mystl::iterator_traits<ForwardIt>::value_type;
        detail::destroy_guard<ValueType> guard{mystl::addressof(*d_first), mystl::addressof(*d_first)};

        for (; first != last; ++first, (void)++d_first)
        {
            mystl::construct_at(mystl::addressof(*d_first), mystl::move_if_noexcept(*first));
            ++guard.current;
        }
        guard.release();
        return d_first;
    }

    // ========================================================================
    // ALLOCATOR TRAITS INTERNAL HELPERS
    // ========================================================================
    namespace detail 
    {
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

        // SFINAE magic: uses mystl::void_t from type_traits.hpp
        template <typename Alloc, typename U>
        struct rebind_alloc_helper<Alloc, U, mystl::void_t<typename Alloc::template rebind<U>::other>> 
        {
            using type = typename Alloc::template rebind<U>::other;
        };

        // --- Optional member-type detection with standard defaults ---
        template <typename A, typename = void> struct at_pointer            { using type = typename A::value_type*; };
        template <typename A> struct at_pointer<A, mystl::void_t<typename A::pointer>>                       { using type = typename A::pointer; };

        template <typename A, typename = void> struct at_const_pointer      { using type = const typename A::value_type*; };
        template <typename A> struct at_const_pointer<A, mystl::void_t<typename A::const_pointer>>           { using type = typename A::const_pointer; };

        template <typename A, typename = void> struct at_void_pointer       { using type = void*; };
        template <typename A> struct at_void_pointer<A, mystl::void_t<typename A::void_pointer>>             { using type = typename A::void_pointer; };

        template <typename A, typename = void> struct at_const_void_pointer { using type = const void*; };
        template <typename A> struct at_const_void_pointer<A, mystl::void_t<typename A::const_void_pointer>> { using type = typename A::const_void_pointer; };

        template <typename A, typename = void> struct at_difference_type    { using type = ptrdiff_t; };
        template <typename A> struct at_difference_type<A, mystl::void_t<typename A::difference_type>>       { using type = typename A::difference_type; };

        template <typename A, typename = void> struct at_size_type          { using type = size_t; };
        template <typename A> struct at_size_type<A, mystl::void_t<typename A::size_type>>                   { using type = typename A::size_type; };

        template <typename A, typename = void> struct at_pocca              { using type = false_type; };
        template <typename A> struct at_pocca<A, mystl::void_t<typename A::propagate_on_container_copy_assignment>> { using type = typename A::propagate_on_container_copy_assignment; };

        template <typename A, typename = void> struct at_pocma              { using type = false_type; };
        template <typename A> struct at_pocma<A, mystl::void_t<typename A::propagate_on_container_move_assignment>> { using type = typename A::propagate_on_container_move_assignment; };

        template <typename A, typename = void> struct at_pocs               { using type = false_type; };
        template <typename A> struct at_pocs<A, mystl::void_t<typename A::propagate_on_container_swap>>      { using type = typename A::propagate_on_container_swap; };

        template <typename A, typename = void> struct at_always_equal       { using type = bool_constant<is_empty_v<A>>; };
        template <typename A> struct at_always_equal<A, mystl::void_t<typename A::is_always_equal>>          { using type = typename A::is_always_equal; };

        // --- Optional member-function detection ---
        template <typename A, typename = void> struct has_max_size : false_type {};
        template <typename A> struct has_max_size<A, mystl::void_t<decltype(mystl::declval<const A&>().max_size())>> : true_type {};

        template <typename A, typename = void> struct has_socc : false_type {};
        template <typename A> struct has_socc<A, mystl::void_t<decltype(mystl::declval<const A&>().select_on_container_copy_construction())>> : true_type {};
    }

    // ========================================================================
    // ALLOCATOR TRAITS
    // ========================================================================
    template <typename Alloc>
    struct allocator_traits
    {
        using allocator_type     = Alloc;
        using value_type         = typename Alloc::value_type;

        using pointer            = typename detail::at_pointer<Alloc>::type;
        using const_pointer      = typename detail::at_const_pointer<Alloc>::type;
        using void_pointer       = typename detail::at_void_pointer<Alloc>::type;
        using const_void_pointer = typename detail::at_const_void_pointer<Alloc>::type;
        using difference_type    = typename detail::at_difference_type<Alloc>::type;
        using size_type          = typename detail::at_size_type<Alloc>::type;

        using propagate_on_container_copy_assignment = typename detail::at_pocca<Alloc>::type;
        using propagate_on_container_move_assignment = typename detail::at_pocma<Alloc>::type;
        using propagate_on_container_swap            = typename detail::at_pocs<Alloc>::type;
        using is_always_equal                        = typename detail::at_always_equal<Alloc>::type;

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

        static size_type max_size(const Alloc& a) noexcept
        {
            if constexpr (detail::has_max_size<Alloc>::value)
                return a.max_size();
            else
                return static_cast<size_type>(-1) / sizeof(value_type);
        }

        static Alloc select_on_container_copy_construction(const Alloc& a)
        {
            if constexpr (detail::has_socc<Alloc>::value)
                return a.select_on_container_copy_construction();
            else
                return a;
        }
    };

} // namespace mystl


// ========================================================================
// SMART POINTERS INCLUSIONS 
// ========================================================================
#include "unique_ptr.hpp"
#include "shared_ptr.hpp" 