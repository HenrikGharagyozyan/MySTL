#pragma once

#include "type_traits.hpp" 

namespace mystl
{
    // ========================================================================
    // MOVE SEMANTICS & PERFECT FORWARDING
    // ========================================================================

    template <typename T>
    constexpr remove_reference_t<T>&& move(T&& arg) noexcept
    {
        return static_cast<remove_reference_t<T>&&>(arg);
    }

    // Moves when T's move constructor is noexcept (or T is not copyable),
    // otherwise yields a const lvalue reference so the source survives a
    // throwing transfer. Foundation for the strong exception guarantee on
    // container reallocation.
    template <typename T>
    constexpr conditional_t<
        !is_nothrow_move_constructible_v<T> && is_copy_constructible_v<T>,
        const T&,
        T&&
    > move_if_noexcept(T& arg) noexcept
    {
        return mystl::move(arg);
    }

    // Produces a value of type T&& in unevaluated contexts only (decltype,
    // sizeof, noexcept). Never defined and never odr-used, mirroring the
    // standard declval. Enables member detection in traits.
    template <typename T>
    T&& declval() noexcept;

    template <typename T>
    constexpr T&& forward(remove_reference_t<T>& arg) noexcept
    {
        return static_cast<T&&>(arg);
    }

    template <typename T>
    constexpr T&& forward(remove_reference_t<T>&& arg) noexcept
    {
        static_assert(
            !is_lvalue_reference<T>::value,
            "mystl::forward: T must not be an lvalue reference"
        );
        return static_cast<T&&>(arg);
    }

    template <typename T> 
    constexpr void swap(T& a, T& b) noexcept
    {
        T temp = mystl::move(a);
        a = mystl::move(b);
        b = mystl::move(temp);
    }

    // ========================================================================
    // EXCHANGE
    // ========================================================================

    template <typename T, typename U = T>
    constexpr T exchange(T& obj, U&& new_value) noexcept
    {
        T old_value = mystl::move(obj);
        obj = mystl::forward<U>(new_value);
        return old_value;
    }

    // ========================================================================
    // Index sequence
    // ========================================================================
    template <std::size_t... Is>
    struct index_sequence {};
    
    template <std::size_t N, std::size_t... Is>
    struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};

    template <std::size_t... Is>
    struct make_index_sequence_impl<0, Is...> 
    {
        using type = index_sequence<Is...>;
    };

    template <std::size_t N>
    using make_index_sequence = typename make_index_sequence_impl<N>::type;

} // namespace mystl