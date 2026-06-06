#pragma once

namespace mystl
{

    // ============================================================================
    // 1. TYPE TRAITS (Type Utilities)
    // ============================================================================

    // remove_reference: removes reference qualifiers from a type (T& -> T, T&& -> T)
    // This is essential for the correct implementation of mystl::move.
    template <typename T> struct remove_reference      { using type = T; };
    template <typename T> struct remove_reference<T&>  { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };

    // Helper alias for C++14/17+
    template <typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    // ============================================================================
    // 2. MOVE SEMANTICS AND PERFECT FORWARDING
    // ============================================================================

    // mystl::move
    // Unconditionally casts the passed object to an rvalue reference (T&&).
    // constexpr noexcept ensures there is no runtime overhead.
    template <typename T>
    constexpr remove_reference_t<T>&& move(T&& arg) noexcept
    {
        return static_cast<remove_reference_t<T>&&>(arg);
    }

    // mystl::forward
    // Preserves the value category (lvalue remains lvalue, rvalue remains rvalue).
    // Crucial for implementing perfect forwarding in constructors and allocators.
    template <typename T>
    constexpr T&& forward(remove_reference_t<T>& arg) noexcept
    {
        return static_cast<T&&>(arg);
    }

    template <typename T>
    constexpr T&& forward(remove_reference_t<T>&& arg) noexcept
    {
        return static_cast<T&&>(arg);
    }

    // ============================================================================
    // 3. mystl::Pair
    // ============================================================================

    template <typename T1, typename T2>
    struct Pair
    {
        T1 first;
        T2 second;

        // Default constructor
        constexpr Pair() 
            : first()
            , second() 
        {
        }

        // Copy/Move constructors
        constexpr Pair(const T1& x, const T2& y) 
            : first(x)
            , second(y) 
        {
        }

        template <typename U1, typename U2>
        constexpr Pair(U1&& x, U2&& y)
            : first(mystl::forward<U1>(x))
            , second(mystl::forward<U2>(y)) 
        {
        }

        // Rule of Five: the compiler will generate efficient move/copy constructors and assignment operators
        Pair(const Pair&) = default;
        Pair(Pair&&) noexcept = default;
        Pair& operator=(const Pair&) = default;
        Pair& operator=(Pair&&) noexcept = default;
        ~Pair() = default;

        // Swap
        void swap(Pair& other) noexcept
        {
            T1 temp1 = mystl::move(first);
            first = mystl::move(other.first);
            other.first = mystl::move(temp1);

            T2 temp2 = mystl::move(second);
            second = mystl::move(other.second);
            other.second = mystl::move(temp2);
        }
    };

    // Helper function make_pair for type deduction
    template <typename T1, typename T2>
    constexpr Pair<remove_reference_t<T1>, remove_reference_t<T2>> make_pair(T1&& x, T2&& y)
    {
        return Pair<remove_reference_t<T1>, remove_reference_t<T2>>(
            mystl::forward<T1>(x), mystl::forward<T2>(y));
    }

} // namespace mystl