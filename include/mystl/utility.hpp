#pragma once

#include <cstddef> // For std::size_t

namespace mystl
{

    // ========================================================================
    // 1. TYPE TRAITS (REFERENCE MODIFIERS)
    // ========================================================================

    template <typename T> struct remove_reference { using type = T; };
    template <typename T> struct remove_reference<T&> { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };

    template <typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    // ========================================================================
    // 2. TYPE TRANSFORMATIONS (CV & DECAY)
    // ========================================================================

    template <typename T> struct remove_const { using type = T; };
    template <typename T> struct remove_const<const T> { using type = T; };

    template <typename T> struct remove_volatile { using type = T; };
    template <typename T> struct remove_volatile<volatile T> { using type = T; };

    template <typename T> 
    struct remove_cv 
    { 
        using type = typename remove_volatile<typename remove_const<T>::type>::type; 
    };
    
    template <typename T> 
    using remove_cv_t = typename remove_cv<T>::type;

    // Hidden implementation details of decay
    namespace detail 
    {
        template <typename T> struct decay_impl { using type = remove_cv_t<T>; };
        // Array decay to pointers
        template <typename T, std::size_t N> struct decay_impl<T[N]> { using type = T*; };
        template <typename T> struct decay_impl<T[]> { using type = T*; };
        // Function decay to function pointers
        template <typename R, typename... Args> struct decay_impl<R(Args...)> { using type = R(*)(Args...); };
    }

    template <typename T> 
    struct decay 
    { 
        using type = typename detail::decay_impl<remove_reference_t<T>>::type; 
    };

    template <typename T> 
    using decay_t = typename decay<T>::type;

    // ========================================================================
    // 3. FUNDAMENTAL TYPE TRAITS (SFINAE)
    // ========================================================================

    template <typename T, typename U> struct is_same { static constexpr bool value = false; };
    template <typename T> struct is_same<T, T> { static constexpr bool value = true; };

    template <typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template <bool B, typename T = void> struct enable_if {};
    template <typename T> struct enable_if<true, T> { using type = T; };

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    // ========================================================================
    // 4. MOVE SEMANTICS & PERFECT FORWARDING
    // ========================================================================

    template <typename T>
    constexpr remove_reference_t<T>&& move(T&& arg) noexcept
    {
        return static_cast<remove_reference_t<T>&&>(arg);
    }

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

    template <typename T> 
    constexpr void swap(T& a, T& b) noexcept
    {
        T temp = mystl::move(a);
        a = mystl::move(b);
        b = mystl::move(temp);
    }

    // ========================================================================
    // 5. mystl::Pair
    // ========================================================================

    template <typename T1, typename T2>
    struct Pair
    {
        T1 first;
        T2 second;

        constexpr Pair() : first(), second() {}

        constexpr Pair(const T1& x, const T2& y) : first(x), second(y) {}

        template <typename U1, typename U2>
        constexpr Pair(U1&& x, U2&& y)
            : first(mystl::forward<U1>(x))
            , second(mystl::forward<U2>(y))
        {
        }

        Pair(const Pair&) = default;               
        Pair(Pair&&) noexcept = default;           
        Pair& operator=(const Pair&) = default;    
        Pair& operator=(Pair&&) noexcept = default;
        ~Pair() = default;                          

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

    // make_pair now uses decay_t
    template <typename T1, typename T2>
    constexpr Pair<decay_t<T1>, decay_t<T2>> make_pair(T1&& x, T2&& y)
    {
        return Pair<decay_t<T1>, decay_t<T2>>(
            mystl::forward<T1>(x), mystl::forward<T2>(y));
    }

    // Comparison operators
    template <typename T1, typename T2>
    constexpr bool operator==(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return lhs.first == rhs.first && lhs.second == rhs.second; 
    }

    template <typename T1, typename T2>
    constexpr bool operator!=(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return !(lhs == rhs); 
    }

    template <typename T1, typename T2>
    constexpr bool operator<(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return lhs.first < rhs.first || (!(rhs.first < lhs.first) && lhs.second < rhs.second); 
    }

    template <typename T1, typename T2>
    constexpr bool operator>(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return rhs < lhs; 
    }

    template <typename T1, typename T2>
    constexpr bool operator<=(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return !(rhs < lhs); 
    }

    template <typename T1, typename T2>
    constexpr bool operator>=(const Pair<T1, T2>& lhs, const Pair<T1, T2>& rhs) 
    { 
        return !(lhs < rhs); 
    }

    template <typename T1, typename T2>
    constexpr void swap(Pair<T1, T2>& lhs, Pair<T1, T2>& rhs) noexcept
    { 
        lhs.swap(rhs); 
    }

} // namespace mystl