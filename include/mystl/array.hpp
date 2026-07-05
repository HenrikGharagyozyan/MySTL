#pragma once

#include "type_traits.hpp"
#include "utility.hpp"
#include <cstddef>    // Для std::size_t
#include <stdexcept>  // Для std::out_of_range

namespace mystl
{
    template <typename T, std::size_t N>
    struct array
    {
        using value_type      = T;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = T&;
        using const_reference = const T&;
        using pointer         = T*;
        using const_pointer   = const T*;
        using iterator        = T*;
        using const_iterator  = const T*;

        T elements[N > 0 ? N : 1];

        constexpr iterator begin() noexcept { return elements; }
        constexpr const_iterator begin() const noexcept { return elements; }
        constexpr const_iterator cbegin() const noexcept { return elements; }

        constexpr iterator end() noexcept { return elements + N; }
        constexpr const_iterator end() const noexcept { return elements + N; }
        constexpr const_iterator cend() const noexcept { return elements + N; }

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


    template <typename T, std::size_t N>
    constexpr void swap(array<T, N>& lhs, array<T, N>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

    template <typename T, std::size_t N>
    constexpr bool operator==(const array<T, N>& lhs, const array<T, N>& rhs)
    {
        for (std::size_t i = 0; i < N; ++i) 
        {
            if (!(lhs[i] == rhs[i])) 
                return false;
        }
        return true;
    }

    template <typename T, std::size_t N>
    constexpr bool operator!=(const array<T, N>& lhs, const array<T, N>& rhs)
    {
        return !(lhs == rhs);
    }


    // ========================================================================
    // Интеграция с Tuple (Structured Bindings)
    // ========================================================================

    // Специализация get<I> для array
    template <std::size_t I, typename T, std::size_t N>
    constexpr T& get(array<T, N>& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return a.elements[I];
    }

    template <std::size_t I, typename T, std::size_t N>
    constexpr const T& get(const array<T, N>& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return a.elements[I];
    }

    template <std::size_t I, typename T, std::size_t N>
    constexpr T&& get(array<T, N>&& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return mystl::move(a.elements[I]);
    }

    template <std::size_t I, typename T, std::size_t N>
    constexpr const T&& get(const array<T, N>&& a) noexcept
    {
        static_assert(I < N, "mystl::get: index out of bounds");
        return mystl::move(a.elements[I]);
    }

    // Предварительные объявления из tuple.hpp
    template <typename TupleType> struct tuple_size;
    template <std::size_t I, typename TupleType> struct tuple_element;

    // Специализация tuple_size для array
    template <typename T, std::size_t N>
    struct tuple_size<array<T, N>> : integral_constant<std::size_t, N> {};

    // Специализация tuple_element для array
    template <std::size_t I, typename T, std::size_t N>
    struct tuple_element<I, array<T, N>>
    {
        static_assert(I < N, "mystl::tuple_element: index out of bounds");
        using type = T;
    };
    
} // namespace mystl