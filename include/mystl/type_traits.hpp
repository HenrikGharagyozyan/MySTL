#pragma once

#include <cstddef> // For std::size_t

namespace mystl
{
    // ========================================================================
    // INTEGRAL CONSTANTS
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
    // REFERENCE MODIFIERS
    // ========================================================================

    template <typename T> struct remove_reference { using type = T; };
    template <typename T> struct remove_reference<T&> { using type = T; };
    template <typename T> struct remove_reference<T&&> { using type = T; };

    template <typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    // ========================================================================
    // CV MODIFIERS (Const / Volatile)
    // ========================================================================

    template <typename T> struct remove_const { using type = T; };
    template <typename T> struct remove_const<const T> { using type = T; };

    template <typename T>
    using remove_const_t = typename remove_const<T>::type;

    template <typename T> struct remove_volatile { using type = T; };
    template <typename T> struct remove_volatile<volatile T> { using type = T; };

    template <typename T> 
    struct remove_cv 
    { 
        using type = typename remove_volatile<typename remove_const<T>::type>::type; 
    };
    
    template <typename T> 
    using remove_cv_t = typename remove_cv<T>::type;

    template <typename T> struct add_const { using type = const T; };
    template <typename T> using add_const_t = typename add_const<T>::type;

    template <typename T> struct add_volatile { using type = volatile T; };
    template <typename T> using add_volatile_t = typename add_volatile<T>::type;

    template <typename T> struct add_cv { using type = typename add_volatile<typename add_const<T>::type>::type; };
    template <typename T> using add_cv_t = typename add_cv<T>::type;

    // ========================================================================
    // POINTER MODIFIERS
    // ========================================================================

    template <typename T> struct remove_pointer { using type = T; };
    template <typename T> struct remove_pointer<T*> { using type = T; };
    template <typename T> struct remove_pointer<T* const> { using type = T; };
    template <typename T> struct remove_pointer<T* volatile> { using type = T; };
    template <typename T> struct remove_pointer<T* const volatile> { using type = T; };

    template <typename T>
    using remove_pointer_t = typename remove_pointer<T>::type;

    template <typename T> struct add_pointer { using type = T*; };
    template <typename T> using add_pointer_t = typename add_pointer<T>::type;

    // ========================================================================
    // DECAY
    // ========================================================================

    namespace detail 
    {
        template <typename T> struct decay_impl { using type = remove_cv_t<T>; };
        template <typename T, std::size_t N> struct decay_impl<T[N]> { using type = T*; };
        template <typename T> struct decay_impl<T[]> { using type = T*; };
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
    // FUNDAMENTAL TYPE PROPERTIES
    // ========================================================================

    template <typename T, typename U> struct is_same : false_type {};
    template <typename T> struct is_same<T, T> : true_type {};

    template <typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template<typename T> struct is_const : false_type {};
    template<typename T> struct is_const<const T> : true_type {};

    template<typename T>
    inline constexpr bool is_const_v = is_const<T>::value;

    template<typename T> struct is_volatile : false_type {};
    template<typename T> struct is_volatile<volatile T> : true_type {};

    template<typename T>
    inline constexpr bool is_volatile_v = is_volatile<T>::value;

    template <typename T> struct is_pointer_helper : false_type {};
    template <typename T> struct is_pointer_helper<T*> : true_type {};
    
    template <typename T> struct is_pointer : is_pointer_helper<remove_cv_t<T>> {};

    template <typename T> struct is_integral : false_type {};
    template <> struct is_integral<bool> : true_type {};
    template <> struct is_integral<char> : true_type {};
    template <> struct is_integral<signed char> : true_type {};
    template <> struct is_integral<unsigned char> : true_type {};
    template <> struct is_integral<short> : true_type {};
    template <> struct is_integral<unsigned short> : true_type {};
    template <> struct is_integral<int> : true_type {};
    template <> struct is_integral<unsigned int> : true_type {};
    template <> struct is_integral<long> : true_type {};
    template <> struct is_integral<unsigned long> : true_type {};
    template <> struct is_integral<long long> : true_type {};
    template <> struct is_integral<unsigned long long> : true_type {};

    template <typename T> struct is_lvalue_reference : false_type {};
    template <typename T> struct is_lvalue_reference<T&> : true_type {};

    template <typename T>
    inline constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

    // ========================================================================
    // SFINAE & CONDITIONAL
    // ========================================================================

    // Maps any well-formed sequence of types to void; the detection idiom's
    // building block (equivalent to std::void_t).
    template <typename...>
    using void_t = void;

    template <bool B, typename T = void> struct enable_if {};
    template <typename T> struct enable_if<true, T> { using type = T; };

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    template <bool B, typename T, typename F> struct conditional { using type = T; };
    template <typename T, typename F> struct conditional<false, T, F> { using type = F; };

    template <bool B, typename T, typename F>
    using conditional_t = typename conditional<B, T, F>::type;

    // ========================================================================
    // COMPILER INTRINSICS (Magicheskie funkcii kompilyatora)
    // ========================================================================
    
    template <typename T>
    inline constexpr bool is_trivially_copy_assignable_v = __is_trivially_assignable(T&, const T&);

    template <typename T>
    inline constexpr bool is_trivially_move_assignable_v = __is_trivially_assignable(T&, T&&);

#if defined(__clang__) || defined(_MSC_VER)
    // Clang / MSVC provide the standard-semantics intrinsic directly.
    template <typename T>
    inline constexpr bool is_trivially_destructible_v = __is_trivially_destructible(T);
#else
    // GCC exposes only the legacy spelling (it has no __is_trivially_destructible
    // builtin); it yields the same result for all destructible types.
    template <typename T>
    inline constexpr bool is_trivially_destructible_v = __has_trivial_destructor(T);
#endif

    template <typename T>
    inline constexpr bool is_copy_constructible_v = __is_constructible(T, const T&);

    template <typename T>
    inline constexpr bool is_move_constructible_v = __is_constructible(T, T&&);

    template <typename T>
    inline constexpr bool is_empty_v = __is_empty(T);

    // Проверяет, можно ли создать объект типа T из аргументов Args
    template <typename T, typename... Args>
    inline constexpr bool is_constructible_v = __is_constructible(T, Args...);

    // Проверяет, можно ли создать объект без выброса исключений (noexcept)
    template <typename T, typename... Args>
    inline constexpr bool is_nothrow_constructible_v = __is_nothrow_constructible(T, Args...);

    // Можно ли переместить объект без исключений
    template <typename T>
    inline constexpr bool is_nothrow_move_constructible_v = __is_nothrow_constructible(T, T&&);

    // Можно ли присвоить объект (перемещением) без исключений
    template <typename T>
    inline constexpr bool is_nothrow_move_assignable_v = __is_nothrow_assignable(T&, T&&);

} // namespace mystl