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

    template <typename T> struct add_const { using type = const T; };
    template <typename T> using add_const_t = typename add_const<T>::type;

    template <typename T> struct add_volatile { using type = volatile T; };
    template <typename T> using add_volatile_t = typename add_volatile<T>::type;

    template <typename T> struct add_cv { using type = typename add_volatile<typename add_const<T>::type>::type; };
    template <typename T> using add_cv_t = typename add_cv<T>::type;

    template <typename T> struct add_pointer { using type = T*; };
    template <typename T> using add_pointer_t = typename add_pointer<T>::type;

    // ========================================================================
    // INTEGRAL CONSTANTS & TAG DISPATCHING
    // ========================================================================

    template <typename T, T v>
    struct integral_constant 
    {
        static constexpr T value = v;
        using value_type = T;
        using type = integral_constant;
        constexpr operator value_type() const noexcept { return value; }
        constexpr value_type operator()() const noexcept { return value; }
    };

    using true_type  = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    template <bool B>
    using bool_constant = integral_constant<bool, B>;

    // ========================================================================
    // REMOVE POINTER
    // ========================================================================

    template <typename T> struct remove_pointer { using type = T; };
    template <typename T> struct remove_pointer<T*> { using type = T; };
    template <typename T> struct remove_pointer<T* const> { using type = T; };
    template <typename T> struct remove_pointer<T* volatile> { using type = T; };
    template <typename T> struct remove_pointer<T* const volatile> { using type = T; };

    template <typename T>
    using remove_pointer_t = typename remove_pointer<T>::type;

    // Alias for remove_const_t (your code has struct remove_const, but no _t)
    template <typename T>
    using remove_const_t = typename remove_const<T>::type;

    // ========================================================================
    // COMPILER INTRINSICS (For safe memmove)
    // ========================================================================
    
    // We use compiler built-in functions (GCC/Clang/MSVC) to check triviality
    template <typename T>
    inline constexpr bool is_trivially_copy_assignable_v = __is_trivially_assignable(T&, const T&);

    template <typename T>
    inline constexpr bool is_trivially_move_assignable_v = __is_trivially_assignable(T&, T&&);


    // ========================================================================
    // 3. FUNDAMENTAL TYPE TRAITS (SFINAE)
    // ========================================================================

    template <typename T, typename U> struct is_same { static constexpr bool value = false; };
    template <typename T> struct is_same<T, T> { static constexpr bool value = true; };

    template <typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template<typename T> struct is_const { static constexpr bool value = false; };
    template<typename T> struct is_const<const T> { static constexpr bool value = true; };

    template<typename T>
    inline constexpr bool is_const_v = is_const<T>::value;

    template<typename T> struct is_volatile { static constexpr bool value = false; };
    template<typename T> struct is_volatile<volatile T> { static constexpr bool value = true; };

    template<typename T>
    inline constexpr bool is_volatile_v = is_volatile<T>::value;

    template <typename T> struct is_pointer { static constexpr bool value = false; };
    template <typename T> struct is_pointer<T*> { static constexpr bool value = true; };
    template <typename T> struct is_pointer<T* const> { static constexpr bool value = true; };
    template <typename T> struct is_pointer<T* volatile> { static constexpr bool value = true; };
    template <typename T> struct is_pointer<T* const volatile> { static constexpr bool value = true; };

    template <bool B, typename T = void> struct enable_if {};
    template <typename T> struct enable_if<true, T> { using type = T; };

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    template <typename T> struct is_integral { static constexpr bool value = false; };
    template <> struct is_integral<bool> { static constexpr bool value = true; };
    template <> struct is_integral<char> { static constexpr bool value = true; };
    template <> struct is_integral<signed char> { static constexpr bool value = true; };
    template <> struct is_integral<unsigned char> { static constexpr bool value = true; };
    template <> struct is_integral<short> { static constexpr bool value = true; };
    template <> struct is_integral<unsigned short> { static constexpr bool value = true; };
    template <> struct is_integral<int> { static constexpr bool value = true; };
    template <> struct is_integral<unsigned int> { static constexpr bool value = true; };
    template <> struct is_integral<long> { static constexpr bool value = true; };
    template <> struct is_integral<unsigned long> { static constexpr bool value = true; };
    template <> struct is_integral<long long> { static constexpr bool value = true; };
    template <> struct is_integral<unsigned long long> { static constexpr bool value = true; };

    template <typename T> struct is_lvalue_reference { static constexpr bool value = false; };
    template <typename T> struct is_lvalue_reference<T&> { static constexpr bool value = true; };

    template <typename T>
    inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

    // ========================================================================
    // Conditional
    // ========================================================================
    template <bool B, typename T, typename F> struct conditional { using type = T; };
    template <typename T, typename F> struct conditional<false, T, F> { using type = F; };

    template <bool B, typename T, typename F>
    using conditional_t = typename conditional<B, T, F>::type;

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
    // Exchange two values
    // ========================================================================

    template <typename T, typename U = T>
    constexpr T exchange(T& obj, U&& new_value) noexcept
    {
        T old_value = mystl::move(obj);
        obj = mystl::forward<U>(new_value);
        return old_value;
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

        constexpr void swap(Pair& other) noexcept
        {
            mystl::swap(first, other.first);
            mystl::swap(second, other.second);
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