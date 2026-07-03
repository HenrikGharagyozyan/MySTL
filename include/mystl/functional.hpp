#pragma once

#include "utility.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace mystl
{
    // ========================================================================
    // 1. ARITHMETIC OPERATIONS (TRANSPARENT)
    // ========================================================================

    struct plus 
    {
        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) + mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) + mystl::forward<U>(rhs);
        }
    };

    struct minus 
    {
        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) - mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) - mystl::forward<U>(rhs);
        }
    };

    struct multiplies 
    {
        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) * mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) * mystl::forward<U>(rhs);
        }
    };

    // ========================================================================
    // 2. COMPARISON OPERATIONS (TRANSPARENT COMPARATORS)
    // ========================================================================

    struct equal_to 
    {
        using is_transparent = int;

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) == mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) == mystl::forward<U>(rhs);
        }
    };

    struct not_equal_to 
    {
        using is_transparent = int;

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) != mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) != mystl::forward<U>(rhs);
        }
    };

    struct greater 
    {
        using is_transparent = int;

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) > mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) > mystl::forward<U>(rhs);
        }
    };

    struct less 
    {
        using is_transparent = int; // Flag for smart searching in Map/Set (without allocations)

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) < mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) < mystl::forward<U>(rhs);
        }
    };

    struct greater_equal 
    {
        using is_transparent = int;

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) >= mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) >= mystl::forward<U>(rhs);
        }
    };

    struct less_equal 
    {
        using is_transparent = int;

        template <typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const
            -> decltype(mystl::forward<T>(lhs) <= mystl::forward<U>(rhs))
        {
            return mystl::forward<T>(lhs) <= mystl::forward<U>(rhs);
        }
    };

    // ========================================================================
    // KEY EXTRACTION POLICIES
    // Shared by the ordered (RBTree) and unordered (HashTable) associative
    // backbones to obtain a key from a stored value.
    // ========================================================================

    template <typename T>
    struct Identity
    {
        const T& operator()(const T& x) const { return x; }
    };

    template <typename Pair>
    struct Select1st
    {
        constexpr const auto& operator()(const Pair& p) const noexcept { return p.first; }
    };

    // ========================================================================
    // 3. REFERENCE WRAPPER
    // Wrapper that allows storing references in containers or passing them by value
    // ========================================================================

    template <typename T>
    class reference_wrapper
    {
    private:
        T* ptr_;

    public:
        using type = T;

        reference_wrapper(T& ref) noexcept : ptr_(&ref) {}
        reference_wrapper(T&&) = delete; // Prohibit binding temporary objects!

        reference_wrapper(const reference_wrapper& other) noexcept = default;
        reference_wrapper& operator=(const reference_wrapper& other) noexcept = default;

        operator T& () const noexcept { return *ptr_; }
        T& get() const noexcept { return *ptr_; }

        template <typename... Args>
        auto operator()(Args&&... args) const
            -> decltype((*ptr_)(mystl::forward<Args>(args)...))
        {
            return (*ptr_)(mystl::forward<Args>(args)...);
        }
    };

    template <typename T>
    reference_wrapper<T> ref(T& t) noexcept
    {
        return reference_wrapper<T>(t);
    }

    template <typename T>
    reference_wrapper<T> ref(reference_wrapper<T> t) noexcept
    {
        return t;
    }

    template <typename T>
    reference_wrapper<const T> cref(const T& t) noexcept
    {
        return reference_wrapper<const T>(t);
    }

    template <typename T>
    void ref(const T&&) = delete;

    template <typename T>
    void cref(const T&&) = delete;

    // ========================================================================
    // 4. HASH FRAMEWORK (Foundation for Unordered containers)
    // ========================================================================

    namespace detail 
    {
        // FNV-1a hashing algorithm
        inline constexpr std::size_t fnv1a_32(const char* s, std::size_t count) noexcept 
        {
            std::size_t hash = 2166136261u;
            for (std::size_t i = 0; i < count; ++i) 
            {
                hash ^= static_cast<std::size_t>(static_cast<unsigned char>(s[i]));
                hash *= 16777619u;
            }
            return hash;
        }

        inline constexpr std::size_t fnv1a_64(const char* s, std::size_t count) noexcept
        {
            std::size_t hash = 14695981039346656037ull;
            for (std::size_t i = 0; i < count; ++i) 
            {
                hash ^= static_cast<std::size_t>(static_cast<unsigned char>(s[i]));
                hash *= 1099511628211ull;
            }
            return hash;
        }

        inline constexpr std::size_t hash_bytes(const void* ptr, std::size_t count) noexcept 
        {
            if constexpr (sizeof(std::size_t) == 8)
                return fnv1a_64(static_cast<const char*>(ptr), count);
            else
                return fnv1a_32(static_cast<const char*>(ptr), count);
        }
    } // namespace detail

    // Base template (raises compilation error for non-hashable types)
    template <typename T>
    struct hash;

    // Macro for fast generation of identity hashes for integer types
    #define MYSTL_GENERATE_INT_HASH(Type) \
        template <> struct hash<Type> { \
            constexpr std::size_t operator()(Type v) const noexcept { \
                return static_cast<std::size_t>(v); \
            } \
        }

    MYSTL_GENERATE_INT_HASH(bool);
    MYSTL_GENERATE_INT_HASH(char);
    MYSTL_GENERATE_INT_HASH(signed char);
    MYSTL_GENERATE_INT_HASH(unsigned char);
    MYSTL_GENERATE_INT_HASH(short);
    MYSTL_GENERATE_INT_HASH(unsigned short);
    MYSTL_GENERATE_INT_HASH(int);
    MYSTL_GENERATE_INT_HASH(unsigned int);
    MYSTL_GENERATE_INT_HASH(long);
    MYSTL_GENERATE_INT_HASH(unsigned long);
    MYSTL_GENERATE_INT_HASH(long long);
    MYSTL_GENERATE_INT_HASH(unsigned long long);

    #undef MYSTL_GENERATE_INT_HASH

    // Specialization for floating-point numbers
    template <>
    struct hash<float> 
    {
        std::size_t operator()(float v) const noexcept 
        {
            return v == 0.0f ? 0 : detail::hash_bytes(&v, sizeof(v));
        }
    };

    template <>
    struct hash<double> 
    {
        std::size_t operator()(double v) const noexcept 
        {
            return v == 0.0 ? 0 : detail::hash_bytes(&v, sizeof(v));
        }
    };

    // Specialization for pointers
    template <typename T>
    struct hash<T*>
    {
        std::size_t operator()(T* ptr) const noexcept
        {
            return reinterpret_cast<std::size_t>(ptr);
        }
    };

    // Specialization for std::string
    template <>
    struct hash<std::string>
    {
        std::size_t operator()(const std::string& s) const noexcept
        {
            return detail::hash_bytes(s.data(), s.size());
        }
    };

    // Specialization for std::string_view (C++17)
    template <>
    struct hash<std::string_view>
    {
        std::size_t operator()(std::string_view sv) const noexcept
        {
            return detail::hash_bytes(sv.data(), sv.size());
        }
    };

} // namespace mystl