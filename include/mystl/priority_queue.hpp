#pragma once

#include "utility.hpp"
#include "algorithm.hpp" // Наши алгоритмы кучи
#include "vector.hpp"   

#include <cassert>

namespace mystl 
{

    template <typename T, typename Container = mystl::Vector<T>, typename Compare = mystl::less<T>>
    class PriorityQueue 
    {
    protected:
        Container c_;
        Compare   comp_;

    public:
        using value_type      = typename Container::value_type;
        using size_type       = typename Container::size_type;
        using reference       = typename Container::reference;
        using const_reference = typename Container::const_reference;

        // ========================================================================
        // КОНСТРУКТОРЫ
        // ========================================================================
        
        PriorityQueue() : c_(), comp_() {}
        
        explicit PriorityQueue(const Compare& comp) : c_(), comp_(comp) {}

        template <typename InputIt>
        PriorityQueue(InputIt first, InputIt last, const Compare& comp = Compare())
            : c_(first, last)
            , comp_(comp) 
        {
            mystl::make_heap(c_.begin(), c_.end(), comp_);
        }

        // ========================================================================
        // ДОСТУП И ВМЕСТИМОСТЬ
        // ========================================================================

        [[nodiscard]] const_reference top() const 
        {
            assert(!empty() && "PriorityQueue is empty!");
            return c_.front();
        }

        [[nodiscard]] bool empty() const noexcept { return c_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return c_.size(); }

        // ========================================================================
        // МОДИФИКАТОРЫ
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