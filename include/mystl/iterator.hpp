#pragma once

#include <cstddef>

#include "type_traits.hpp"

namespace mystl
{
    // ========================================================================
    // ITERATOR TAGS
    // ========================================================================

    struct input_iterator_tag {};
    struct output_iterator_tag {};

    struct forward_iterator_tag : input_iterator_tag {};
    struct bidirectional_iterator_tag : forward_iterator_tag {};
    struct random_access_iterator_tag : bidirectional_iterator_tag {};

    // ========================================================================
    // BASE ITERATOR CLASS
    // ========================================================================

    template<
        typename Category,
        typename T,
        typename Distance = std::ptrdiff_t,
        typename Pointer = T*,
        typename Reference = T&
    >
    struct iterator
    {
        using iterator_category = Category;
        using value_type        = T;
        using difference_type   = Distance;
        using pointer           = Pointer;
        using reference         = Reference;
    };

    // ========================================================================
    // ITERATOR TRAITS
    // ========================================================================

    namespace detail
    {
        // Defines the five member typedefs only when the iterator provides all of
        // them; otherwise stays empty, making iterator_traits SFINAE-friendly for
        // non-iterator types (matching C++17 std::iterator_traits).
        template <typename It, typename = void>
        struct iterator_traits_base {};

        template <typename It>
        struct iterator_traits_base<It, mystl::void_t<
            typename It::iterator_category,
            typename It::value_type,
            typename It::difference_type,
            typename It::pointer,
            typename It::reference>>
        {
            using difference_type   = typename It::difference_type;
            using value_type        = typename It::value_type;
            using pointer           = typename It::pointer;
            using reference         = typename It::reference;
            using iterator_category = typename It::iterator_category;
        };
    }

    template <typename Iterator>
    struct iterator_traits : detail::iterator_traits_base<Iterator> {};

    template <typename T>
    struct iterator_traits<T*>
    {
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;
        using iterator_category = random_access_iterator_tag;
    };

    template <typename T>
    struct iterator_traits<const T*>
    {
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = const T*;
        using reference         = const T&;
        using iterator_category = random_access_iterator_tag;
    };

    // ========================================================================
    // DETAIL IMPLEMENTATIONS (TAG DISPATCHING)
    // ========================================================================
    
    namespace detail
    {
        // --- DISTANCE IMPL ---
        template<typename Iterator>
        typename iterator_traits<Iterator>::difference_type
        distance_impl(Iterator first, Iterator last, input_iterator_tag)
        {
            typename iterator_traits<Iterator>::difference_type n = 0;
            while (first != last)
            {
                ++first;
                ++n;
            }
            return n;
        }

        template<typename Iterator>
        typename iterator_traits<Iterator>::difference_type
        distance_impl(Iterator first, Iterator last, random_access_iterator_tag)
        {
            return last - first;
        }

        // --- ADVANCE IMPL ---
        template<typename Iterator, typename Distance>
        void advance_impl(Iterator& it, Distance n, input_iterator_tag)
        {
            while (n > 0)
            {
                ++it;
                --n;
            }
        }

        template<typename Iterator, typename Distance>
        void advance_impl(Iterator& it, Distance n, bidirectional_iterator_tag)
        {
            if (n >= 0)
            {
                while (n--)
                {
                    ++it;
                }
            }
            else
            {
                while (n++)
                {
                    --it;
                }
            }
        }

        template<typename Iterator, typename Distance>
        void advance_impl(Iterator& it, Distance n, random_access_iterator_tag)
        {
            it += n;
        }
    } // namespace detail

    // ========================================================================
    // PUBLIC INTERFACE: DISTANCE & ADVANCE
    // ========================================================================

    template<typename Iterator>
    typename iterator_traits<Iterator>::difference_type
    distance(Iterator first, Iterator last)
    {
        return detail::distance_impl(
            first,
            last,
            typename iterator_traits<Iterator>::iterator_category{}
        );
    }

    template<typename Iterator, typename Distance>
    void advance(Iterator& it, Distance n)
    {
        detail::advance_impl(
            it,
            n,
            typename iterator_traits<Iterator>::iterator_category{}
        );
    }

    // ========================================================================
    // NEXT & PREV
    // ========================================================================

    template <typename Iterator>
    Iterator next(Iterator it)
    {
        ++it;
        return it;
    }

    template <typename Iterator>
    Iterator next(Iterator it, typename iterator_traits<Iterator>::difference_type n)
    {
        mystl::advance(it, n);
        return it;
    }

    template <typename BidirectionalIterator>
    BidirectionalIterator prev(BidirectionalIterator it)
    {
        --it;
        return it;
    }

    template <typename BidirectionalIterator>
    BidirectionalIterator prev(
        BidirectionalIterator it,
        typename iterator_traits<BidirectionalIterator>::difference_type n)
    {
        mystl::advance(it, -n); // Optimization: leveraging the power of tag dispatching!
        return it;
    }

    // ========================================================================
    // REVERSE ITERATOR
    // ========================================================================

    template <typename Iterator>
    class reverse_iterator
    {
    public:
        using iterator_type     = Iterator;
        using traits_type       = iterator_traits<Iterator>;

        using iterator_category = typename traits_type::iterator_category;
        using value_type        = typename traits_type::value_type;
        using difference_type   = typename traits_type::difference_type;
        using pointer           = typename traits_type::pointer;
        using reference         = typename traits_type::reference;

    private:
        Iterator current_;

    public:
        constexpr reverse_iterator() : current_() {}

        explicit constexpr reverse_iterator(Iterator it) : current_(it) {}

        constexpr Iterator base() const { return current_; }

        constexpr reference operator*() const
        {
            Iterator tmp = current_;
            --tmp;
            return *tmp;
        }

        constexpr pointer operator->() const { return &(operator*()); }

        constexpr reverse_iterator& operator++()
        {
            --current_;
            return *this;
        }

        constexpr reverse_iterator operator++(int)
        {
            reverse_iterator tmp(*this);
            --current_;
            return tmp;
        }

        constexpr reverse_iterator& operator--()
        {
            ++current_;
            return *this;
        }

        constexpr reverse_iterator operator--(int)
        {
            reverse_iterator tmp(*this);
            ++current_;
            return tmp;
        }

        // --- Arithmetic ---

        constexpr reverse_iterator operator+(difference_type n) const
        {
            return reverse_iterator(current_ - n);
        }

        constexpr reverse_iterator operator-(difference_type n) const
        {
            return reverse_iterator(current_ + n);
        }

        constexpr reverse_iterator& operator+=(difference_type n)
        {
            current_ -= n;
            return *this;
        }

        constexpr reverse_iterator& operator-=(difference_type n)
        {
            current_ += n;
            return *this;
        }

        constexpr reference operator[](difference_type n) const
        {
            return *(*this + n);
        }

        constexpr difference_type operator-(const reverse_iterator& other) const
        {
            return other.current_ - current_;
        }

        // --- Comparisons ---

        constexpr bool operator==(const reverse_iterator& other) const
        {
            return current_ == other.current_;
        }

        constexpr bool operator!=(const reverse_iterator& other) const
        {
            return current_ != other.current_;
        }

        constexpr bool operator<(const reverse_iterator& other) const
        {
            return other.current_ < current_;
        }

        constexpr bool operator>(const reverse_iterator& other) const
        {
            return other < *this;
        }

        constexpr bool operator<=(const reverse_iterator& other) const
        {
            return !(other < *this);
        }

        constexpr bool operator>=(const reverse_iterator& other) const
        {
            return !(*this < other);
        }
    };

} // namespace mystl