#pragma once

#include "type_traits.hpp"
#include "utility.hpp" 

namespace mystl
{

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

    template <typename T1, typename T2>
    constexpr Pair<decay_t<T1>, decay_t<T2>> make_pair(T1&& x, T2&& y)
    {
        return Pair<decay_t<T1>, decay_t<T2>>(
            mystl::forward<T1>(x), mystl::forward<T2>(y));
    }

    // ========================================================================
    // GLOBAL OPERATORS AND FUNCTIONS
    // ========================================================================

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