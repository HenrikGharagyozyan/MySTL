#pragma once

#include "type_traits.hpp"
#include "utility.hpp"
#include "cstddef.hpp"


namespace mystl
{
    // ========================================================================
    // 1. TupleLeaf: Storage for a single element
    // ========================================================================
    template <size_t I, typename T>
    struct TupleLeaf
    {
        using value_type = T;

        T value;

        constexpr TupleLeaf() = default;

        template <typename U, typename = enable_if_t<is_constructible_v<T, U&&>>>
        constexpr explicit TupleLeaf(U&& v) noexcept(is_nothrow_constructible_v<T, U&&>)
            : value(mystl::forward<U>(v)) 
        {
        }

        constexpr TupleLeaf(const TupleLeaf&) = default;
        constexpr TupleLeaf(TupleLeaf&&) noexcept(is_nothrow_move_constructible_v<T>) = default;

        constexpr TupleLeaf& operator=(const TupleLeaf&) = default;
        constexpr TupleLeaf& operator=(TupleLeaf&&) noexcept(is_nothrow_move_assignable_v<T>) = default;

        constexpr T& get() & noexcept { return value; }
        constexpr const T& get() const& noexcept { return value; }
        constexpr T&& get() && noexcept { return mystl::move(value); }
        constexpr const T&& get() const&& noexcept { return mystl::move(value); }
    };

    // ========================================================================
    // 2. TupleImpl: Base class inheriting all leaves
    // ========================================================================
    template <typename IndexSeq, typename... Ts>
    struct TupleImpl;

    template <size_t... Is, typename... Ts>
    struct TupleImpl<index_sequence<Is...>, Ts...> : TupleLeaf<Is, Ts>...
    {
        using self_type = TupleImpl<index_sequence<Is...>, Ts...>;

        constexpr TupleImpl() = default;

        template <typename... Us, typename = enable_if_t<sizeof...(Us) == sizeof...(Ts)>>
        constexpr explicit TupleImpl(Us&&... args)
            : TupleLeaf<Is, Ts>(mystl::forward<Us>(args))... 
        {
        }

        constexpr TupleImpl(const self_type&) = default;
        constexpr TupleImpl(self_type&&) noexcept((is_nothrow_move_constructible_v<Ts> && ...)) = default;

        constexpr self_type& operator=(const self_type&) = default;
        constexpr self_type& operator=(self_type&&) noexcept((is_nothrow_move_assignable_v<Ts> && ...)) = default;
    };

    // ========================================================================
    // 3. Tuple: Public interface
    // ========================================================================
    template <typename... Ts>
    struct Tuple : TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...>
    {
        using base_type = TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...>;
        using base_type::base_type; // Inherit constructors
    };

    // ========================================================================
    // 4. Access traits: tuple_element and tuple_size
    // ========================================================================
    template <size_t I, typename TupleType>
    struct tuple_element;

    template <size_t I, typename T, typename... Ts>
    struct tuple_element<I, Tuple<T, Ts...>> : tuple_element<I - 1, Tuple<Ts...>> {};

    template <typename T, typename... Ts>
    struct tuple_element<0, Tuple<T, Ts...>> { using type = T; };

    template <size_t I, typename TupleType>
    using tuple_element_t = typename tuple_element<I, TupleType>::type;

    template <typename TupleType>
    struct tuple_size;

    template <typename... Ts>
    struct tuple_size<Tuple<Ts...>> : integral_constant<size_t, sizeof...(Ts)> {};

    template <typename TupleType>
    inline constexpr size_t tuple_size_v = tuple_size<TupleType>::value;

    // ========================================================================
    // 5. get<I>
    // ========================================================================
    template <size_t I, typename... Ts>
    constexpr tuple_element_t<I, Tuple<Ts...>>& get(Tuple<Ts...>& t) noexcept
    {
        return static_cast<TupleLeaf<I, tuple_element_t<I, Tuple<Ts...>>>&>(t).get();
    }

    template <size_t I, typename... Ts>
    constexpr const tuple_element_t<I, Tuple<Ts...>>& get(const Tuple<Ts...>& t) noexcept
    {
        return static_cast<const TupleLeaf<I, tuple_element_t<I, Tuple<Ts...>>>&>(t).get();
    }

    template <size_t I, typename... Ts>
    constexpr tuple_element_t<I, Tuple<Ts...>>&& get(Tuple<Ts...>&& t) noexcept
    {
        return static_cast<TupleLeaf<I, tuple_element_t<I, Tuple<Ts...>>>&&>(t).get();
    }

    template <size_t I, typename... Ts>
    constexpr const tuple_element_t<I, Tuple<Ts...>>&& get(const Tuple<Ts...>&& t) noexcept
    {
        return static_cast<const TupleLeaf<I, tuple_element_t<I, Tuple<Ts...>>>&&>(t).get();
    }

    // ========================================================================
    // 6. make_tuple
    // ========================================================================
    template <typename... Ts>
    constexpr auto make_tuple(Ts&&... args)
    {
        using ResTuple = Tuple<decay_t<Ts>...>;
        return ResTuple(mystl::forward<Ts>(args)...);
    }

    // ========================================================================
    // 7. Comparison operators
    // ========================================================================
    namespace detail
    {
        template <typename... Ts, size_t... Is>
        constexpr bool tuple_equal_impl(const Tuple<Ts...>& a, const Tuple<Ts...>& b, index_sequence<Is...>)
        {
            return ((mystl::get<Is>(a) == mystl::get<Is>(b)) && ...);
        }

        // Recursive helper for lexicographical comparison (operator<)
        template <size_t I, size_t Size>
        struct TupleCompare
        {
            template <typename T, typename U>
            static constexpr bool less(const T& a, const U& b)
            {
                if (mystl::get<I>(a) < mystl::get<I>(b)) return true;
                if (mystl::get<I>(b) < mystl::get<I>(a)) return false;
                return TupleCompare<I + 1, Size>::less(a, b);
            }
        };

        // Base case of recursion
        template <size_t Size>
        struct TupleCompare<Size, Size>
        {
            template <typename T, typename U>
            static constexpr bool less(const T&, const U&) { return false; }
        };
    } // namespace detail

    template <typename... Ts>
    constexpr bool operator==(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return detail::tuple_equal_impl(a, b, make_index_sequence<sizeof...(Ts)>{});
    }

    template <typename... Ts>
    constexpr bool operator!=(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return !(a == b);
    }

    template <typename... Ts>
    constexpr bool operator<(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return detail::TupleCompare<0, sizeof...(Ts)>::less(a, b);
    }

    template <typename... Ts>
    constexpr bool operator>(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return b < a;
    }

    template <typename... Ts>
    constexpr bool operator<=(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return !(b < a);
    }

    template <typename... Ts>
    constexpr bool operator>=(const Tuple<Ts...>& a, const Tuple<Ts...>& b)
    {
        return !(a < b);
    }

} // namespace mystl