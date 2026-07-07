#pragma once

#include "type_traits.hpp"
#include "utility.hpp"
#include "iterator.hpp" 
#include "cstddef.hpp"
#include <stdexcept>  // For std::out_of_range

namespace mystl
{
    template <typename T, size_t N>
    struct array
    {
        using value_type      = T;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;
        using iterator        = T*;
        using const_iterator  = const T*;

        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

        T elements[N > 0 ? N : 1];

        constexpr iterator begin() noexcept { return elements; }
        constexpr const_iterator begin() const noexcept { return elements; }
        constexpr const_iterator cbegin() const noexcept { return elements; }

        constexpr iterator end() noexcept { return elements + N; }
        constexpr const_iterator end() const noexcept { return elements + N; }
        constexpr const_iterator cend() const noexcept { return elements + N; }

        constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

        constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

        constexpr reference operator[](size_type pos) noexcept 
        { 
            return elements[pos]; 
        }
        
        constexpr const_reference operator[](size_type pos) const noexcept 
        { 
            return elements[pos]; 
        }

        constexpr reference at(size_type pos)
        {
            if (pos >= N) 
                throw std::out_of_range("mystl::array::at: index out of range");
            return elements[pos];
        }

        constexpr const_reference at(size_type pos) const
        {
            if (pos >= N) 
                throw std::out_of_range("mystl::array::at: index out of range");
            return elements[pos];
        }

        constexpr reference front() noexcept { return elements[0]; }
        constexpr const_reference front() const noexcept { return elements[0]; }

        constexpr reference back() noexcept { return elements[N > 0 ? N - 1 : 0]; }
        constexpr const_reference back() const noexcept { return elements[N > 0 ? N - 1 : 0]; }

        constexpr pointer data() noexcept { return elements; }
        constexpr const_pointer data() const noexcept { return elements; }

        constexpr bool empty() const noexcept { return N == 0; }
        constexpr size_type size() const noexcept { return N; }
        constexpr size_type max_size() const noexcept { return N; }

        constexpr void fill(const T& value)
        {
            for (size_type i = 0; i < N; ++i) 
                elements[i] = value;
        }

        constexpr void swap(array& other) noexcept(mystl::is_nothrow_move_constructible_v<T> && mystl::is_nothrow_move_assignable_v<T>)
        {
            for (size_type i = 0; i < N; ++i) 
                mystl::swap(elements[i], other.elements[i]);
        }
    };


    template <typename T, size_t N>
    constexpr void swap(array<T, N>& lhs, array<T, N>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template <typename T, size_t N>
    constexpr bool operator==(const array<T, N>& lhs, const array<T, N>& rhs)
    {
        for (size_t i = 0; i < N; ++i) 
        {
            if (!(lhs[i] == rhs[i])) 
                return false;
        }
        return true;
    }

    template <typename T, size_t N>
    constexpr bool operator!=(const array<T, N>& lhs, const array<T, N>& rhs)
    {
        return !(lhs == rhs);
    }


    // ========================================================================
    // Integration with Tuple (Structured Bindings)
    // ========================================================================

    // Specialization of get<I> for array
    template <size_t I, typename T, size_t N>
    constexpr T& get(array<T, N>& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return a.elements[I];
    }

    template <size_t I, typename T, size_t N>
    constexpr const T& get(const array<T, N>& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return a.elements[I];
    }

    template <size_t I, typename T, size_t N>
    constexpr T&& get(array<T, N>&& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return mystl::move(a.elements[I]);
    }

    template <size_t I, typename T, size_t N>
    constexpr const T&& get(const array<T, N>&& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return mystl::move(a.elements[I]);
    }

    // Forward declarations from tuple.hpp
    template <typename TupleType> struct tuple_size;
    template <size_t I, typename TupleType> struct tuple_element;

    // Specialization of tuple_size for array
    template <typename T, size_t N>
    struct tuple_size<array<T, N>> : integral_constant<size_t, N> {};

    // Specialization of tuple_element for array
    template <size_t I, typename T, size_t N>
    struct tuple_element<I, array<T, N>>
    {
        static_assert(I < N, "mystl::tuple_element: index out of bounds");
        using type = T;
    };

} // namespace mystl


// ========================================================================
// Injection into namespace std (C++ standard requirement)
// ========================================================================
namespace std
{
    // Forward declaration of standard structures (in case <utility> is not included)
    template <typename T> struct tuple_size;
    template <size_t I, typename T> struct tuple_element;

    // Specialization of tuple_size for mystl::array
    template <typename T, size_t N>
    struct tuple_size<mystl::array<T, N>> : mystl::integral_constant<size_t, N> {};

    // Specialization of tuple_element for mystl::array
    template <size_t I, typename T, size_t N>
    struct tuple_element<I, mystl::array<T, N>>
    {
        static_assert(I < N, "std::tuple_element: index out of bounds");
        using type = T;
    };
}