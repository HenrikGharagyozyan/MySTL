#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "cstddef.hpp"
#include <cstring> // For std::memmove

namespace mystl 
{

    // ========================================================================
    // NON-MODIFYING ALGORITHMS
    // ========================================================================

    template <typename InputIt, typename UnaryFunction>
    constexpr UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f)
    {
        for (; first != last; ++first)
        {
            f(*first);
        }
        return f;
    }

    template <typename InputIt, typename T>
    constexpr InputIt find(InputIt first, InputIt last, const T& value)
    {
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                return first;
            }
        }
        return last;
    }

    template <typename InputIt, typename UnaryPredicate>
    constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate p)
    {
        for (; first != last; ++first)
        {
            if (p(*first))
            {
                return first;
            }
        }
        return last;
    }

    template <typename InputIt, typename UnaryPredicate>
    constexpr bool all_of(InputIt first, InputIt last, UnaryPredicate p)
    {
        return mystl::find_if(first, last, [&p](const auto& v) { return !p(v); }) == last;
    }

    template <typename InputIt, typename UnaryPredicate>
    constexpr bool any_of(InputIt first, InputIt last, UnaryPredicate p)
    {
        return mystl::find_if(first, last, p) != last;
    }

    template <typename InputIt, typename UnaryPredicate>
    constexpr bool none_of(InputIt first, InputIt last, UnaryPredicate p)
    {
        return mystl::find_if(first, last, p) == last;
    }

    template <typename InputIt, typename T>
    constexpr typename iterator_traits<InputIt>::difference_type 
    count(InputIt first, InputIt last, const T& value)
    {
        typename iterator_traits<InputIt>::difference_type ret = 0;
        for (; first != last; ++first)
        {
            if (*first == value)
            {
                ++ret;
            }
        }
        return ret;
    }

    // ========================================================================
    // COMPARISONS AND MAX/MIN
    // ========================================================================

    template <typename T>
    constexpr const T& max(const T& a, const T& b)
    {
        return (a < b) ? b : a;
    }

    template <typename T>
    constexpr const T& min(const T& a, const T& b)
    {
        return (b < a) ? b : a;
    }

    template <typename T>
    constexpr const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }

    template <typename InputIt1, typename InputIt2>
    constexpr bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2)
    {
        for (; first1 != last1; ++first1, (void) ++first2)
        {
            if (!(*first1 == *first2))
            {
                return false;
            }
        }
        return true;
    }

    template <typename InputIt1, typename InputIt2>
    constexpr bool lexicographical_compare(InputIt1 first1, InputIt1 last1,
                                           InputIt2 first2, InputIt2 last2)
    {
        for (; (first1 != last1) && (first2 != last2); ++first1, (void) ++first2)
        {
            if (*first1 < *first2) return true;
            if (*first2 < *first1) return false;
        }
        return (first1 == last1) && (first2 != last2);
    }

    // ========================================================================
    // MODIFYING ALGORITHMS (WITH MEMORY OPTIMIZATION)
    // ========================================================================

    namespace detail
    {
        // Basic copy for ordinary iterators
        template <typename InputIt, typename OutputIt>
        constexpr OutputIt copy_impl(InputIt first, InputIt last, OutputIt d_first, mystl::false_type)
        {
            for (; first != last; ++first, (void) ++d_first)
            {
                *d_first = *first;
            }
            return d_first;
        }

        // OPTIMIZATION: memmove for raw pointers to trivial types
        template <typename T, typename U>
        U* copy_impl(T* first, T* last, U* d_first, mystl::true_type)
        {
            if (first != last)
            {
                std::memmove(d_first, first, (last - first) * sizeof(T));
            }
            return d_first + (last - first);
        }

        // Basic move for ordinary iterators
        template <typename InputIt, typename OutputIt>
        constexpr OutputIt move_impl(InputIt first, InputIt last, OutputIt d_first, mystl::false_type)
        {
            for (; first != last; ++first, (void) ++d_first)
            {
                *d_first = mystl::move(*first);
            }
            return d_first;
        }

        // OPTIMIZATION: memmove for raw pointers (since moving trivial types equals copying)
        template <typename T, typename U>
        U* move_impl(T* first, T* last, U* d_first, mystl::true_type)
        {
            if (first != last)
            {
                std::memmove(d_first, first, (last - first) * sizeof(T));
            }
            return d_first + (last - first);
        }
    } // namespace detail

    template <typename InputIt, typename OutputIt>
    inline OutputIt copy(InputIt first, InputIt last, OutputIt d_first)
    {
        using is_fast = mystl::bool_constant<
            mystl::is_pointer<InputIt>::value &&
            mystl::is_pointer<OutputIt>::value &&
            mystl::is_same_v<mystl::remove_const_t<mystl::remove_pointer_t<InputIt>>, mystl::remove_pointer_t<OutputIt>> &&
            mystl::is_trivially_copy_assignable_v<mystl::remove_pointer_t<OutputIt>>
        >;
        return detail::copy_impl(first, last, d_first, is_fast{});
    }

    template <typename InputIt, typename OutputIt>
    inline OutputIt move(InputIt first, InputIt last, OutputIt d_first)
    {
        using is_fast = mystl::bool_constant<
            mystl::is_pointer<InputIt>::value &&
            mystl::is_pointer<OutputIt>::value &&
            mystl::is_same_v<mystl::remove_const_t<mystl::remove_pointer_t<InputIt>>, mystl::remove_pointer_t<OutputIt>> &&
            mystl::is_trivially_move_assignable_v<mystl::remove_pointer_t<OutputIt>>
        >;
        return detail::move_impl(first, last, d_first, is_fast{});
    }

    template <typename ForwardIt, typename T>
    constexpr void fill(ForwardIt first, ForwardIt last, const T& value)
    {
        for (; first != last; ++first)
        {
            *first = value;
        }
    }

    template <typename InputIt, typename OutputIt, typename UnaryOperation>
    constexpr OutputIt transform(InputIt first, InputIt last, OutputIt d_first, UnaryOperation op)
    {
        for (; first != last; ++first, (void) ++d_first)
        {
            *d_first = op(*first);
        }
        return d_first;
    }

    // Helper function for sift-down (O(log N))
    template <typename RandomIt, typename Distance, typename Compare>
    void sift_down(RandomIt first, Distance len, Distance start, Compare comp) 
    {
        Distance parent = start;
        // Remember the sifted element (via move semantics)
        auto value = mystl::move(*(first + parent)); 

        while (true) 
        {
            Distance child = 2 * parent + 1; // Left child
            if (child >= len) 
                break;

            // Select the larger child
            if (child + 1 < len && comp(*(first + child), *(first + child + 1))) 
            {
                child++;
            }

            // If the parent is smaller than the child, move the child up
            if (comp(value, *(first + child))) 
            {
                *(first + parent) = mystl::move(*(first + child));
                parent = child;
            } 
            else 
            {
                break;
            }
        }
        // Place the element in its rightful position
        *(first + parent) = mystl::move(value); 
    }

    template <typename RandomIt, typename Compare>
    void push_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        using Distance = mystl::ptrdiff_t;
        Distance len = last - first;
        if (len < 2) 
            return;

        Distance child = len - 1;
        auto value = mystl::move(*(last - 1));

        // Sift-up (O(log N))
        while (child > 0) 
        {
            Distance parent = (child - 1) / 2;
            if (comp(*(first + parent), value)) 
            {
                *(first + child) = mystl::move(*(first + parent));
                child = parent;
            } 
            else 
            {
                break;
            }
        }
        *(first + child) = mystl::move(value);
    }

    template <typename RandomIt, typename Compare>
    void pop_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        if (last - first < 2) return;
        // Swap the root (first element) with the last one
        mystl::swap(*first, *(last - 1));
        // Restore the heap property for the remaining elements
        sift_down(first, static_cast<mystl::ptrdiff_t>(last - first - 1), static_cast<mystl::ptrdiff_t>(0), comp);
    }

    template <typename RandomIt, typename Compare>
    void make_heap(RandomIt first, RandomIt last, Compare comp) 
    {
        using Distance = mystl::ptrdiff_t;
        Distance len = last - first;
        if (len < 2) 
            return;

        // Start from the last node that has children and move toward the root (O(N))
        for (Distance i = (len - 2) / 2; i >= 0; --i) 
        {
            sift_down(first, len, i, comp);
        }
    }

} // namespace mystl