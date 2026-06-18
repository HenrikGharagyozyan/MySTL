#pragma once

#include "utility.hpp"
#include "algorithm.hpp" // Our heap algorithms
#include "vector.hpp"   

#include <cassert>
#include <memory>
#include <type_traits>

namespace mystl 
{

    template <typename T, typename Container = mystl::Vector<T>, typename Compare = mystl::less<T>>
    class PriorityQueue 
    {
    protected:
        Container c_;
        Compare   comp_;

    public:
        using value_type        = typename Container::value_type;
        using size_type         = typename Container::size_type;
        using reference         = typename Container::reference;
        using const_reference   = typename Container::const_reference;
        using allocator_type    = typename Container::allocator_type;

        static_assert(mystl::is_same<T, value_type>::value,
                      "PriorityQueue<T, Container>: Container::value_type must be T");

        // ========================================================================
        // CONSTRUCTORS
        // ========================================================================
        
        PriorityQueue() : c_(), comp_() {}
        
        explicit PriorityQueue(const Compare& comp) : c_(), comp_(comp) {}

        explicit PriorityQueue(const allocator_type& alloc)
            : c_(alloc)
            , comp_()
        {
        }

        PriorityQueue(const Compare& comp, const allocator_type& alloc)
            : c_(alloc)
            , comp_(comp)
        {
        }

        PriorityQueue(const Compare& comp, const Container& cont)
            : c_(cont)
            , comp_(comp)
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        PriorityQueue(const Compare& comp, Container&& cont)
            : c_(mystl::move(cont))
            , comp_(comp)
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        PriorityQueue(const Compare& comp, const Container& cont, const allocator_type& alloc)
            : c_(cont, alloc)
            , comp_(comp)
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        PriorityQueue(const Compare& comp, Container&& cont, const allocator_type& alloc)
            : c_(mystl::move(cont), alloc)
            , comp_(comp)
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        PriorityQueue(const PriorityQueue& other, const allocator_type& alloc)
            : c_(other.c_, alloc)
            , comp_(other.comp_)
        {
        }

        PriorityQueue(PriorityQueue&& other, const allocator_type& alloc)
            : c_(mystl::move(other.c_), alloc)
            , comp_(mystl::move(other.comp_))
        {
        }

        template <typename InputIt>
        PriorityQueue(InputIt first, InputIt last, const Compare& comp = Compare())
            : c_(first, last)
            , comp_(comp) 
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        template <typename InputIt>
        PriorityQueue(InputIt first, InputIt last, const Compare& comp, const allocator_type& alloc)
            : c_(first, last, alloc)
            , comp_(comp)
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        // ========================================================================
        // ACCESS AND CAPACITY
        // ========================================================================

        [[nodiscard]] const_reference top() const 
        {
            assert(!empty() && "PriorityQueue is empty!");
            return c_.front();
        }

        [[nodiscard]] bool empty() const noexcept { return c_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return c_.size(); }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return c_.get_allocator(); }

        // ========================================================================
        // MODIFIERS
        // ========================================================================

        void push(const value_type& value) 
        {
            c_.push_back(value);
            mystl::push_heap(c_.begin(), c_.end(), comp_);
        }

        void push(value_type&& value) 
        {
            c_.push_back(mystl::move(value));
            mystl::push_heap(c_.begin(), c_.end(), comp_);
        }

        template <typename... Args>
        void emplace(Args&&... args) 
        {
            c_.emplace_back(mystl::forward<Args>(args)...);
            mystl::push_heap(c_.begin(), c_.end(), comp_);
        }

        void pop() 
        {
            assert(!empty() && "PriorityQueue is empty!");
            mystl::pop_heap(c_.begin(), c_.end(), comp_);
            c_.pop_back();
        }

        void swap(PriorityQueue& other) noexcept(noexcept(mystl::swap(c_, other.c_)) && 
                                                 noexcept(mystl::swap(comp_, other.comp_))) 
        {
            mystl::swap(c_, other.c_);
            mystl::swap(comp_, other.comp_);
        }
    };

} // namespace mystl

namespace std
{
    template <typename T, typename Container, typename Compare, typename Alloc>
    struct uses_allocator<mystl::PriorityQueue<T, Container, Compare>, Alloc>
        : std::uses_allocator<Container, Alloc>
    {
    };
}
