#pragma once

#include "deque.hpp"
#include "utility.hpp"
#include "type_traits.hpp"


namespace mystl 
{

    // By default, mystl::Deque is used, but Vector or List can be passed instead
    template <typename T, typename Container = mystl::Deque<T>>
    class Stack 
    {
    public:
        using container_type  = Container;
        using value_type      = typename Container::value_type;
        using size_type       = typename Container::size_type;
        using reference       = typename Container::reference;
        using const_reference = typename Container::const_reference;
        using allocator_type  = typename Container::allocator_type;

        static_assert(mystl::is_same_v<T, value_type>,
                      "Stack<T, Container>: Container::value_type must be T");

    protected:
        Container c_; // Internal container

    public:
        // ========================================================================
        // CONSTRUCTORS
        // ========================================================================
        Stack() : c_() {}
        explicit Stack(const allocator_type& alloc) : c_(alloc) {}
        explicit Stack(const Container& cont) : c_(cont) {}
        explicit Stack(Container&& cont) : c_(mystl::move(cont)) {}
        Stack(const Container& cont, const allocator_type& alloc) : c_(cont, alloc) {}
        Stack(Container&& cont, const allocator_type& alloc) : c_(mystl::move(cont), alloc) {}
        Stack(const Stack& other, const allocator_type& alloc) : c_(other.c_, alloc) {}
        Stack(Stack&& other, const allocator_type& alloc) : c_(mystl::move(other.c_), alloc) {}
        
        Stack(const Stack& other) = default;
        Stack(Stack&& other) noexcept = default;
        Stack& operator=(const Stack& other) = default;
        Stack& operator=(Stack&& other) noexcept = default;
        ~Stack() = default;

        // ========================================================================
        // ELEMENT ACCESS
        // ========================================================================
        [[nodiscard]] bool empty() const { return c_.empty(); }
        [[nodiscard]] size_type size() const { return c_.size(); }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return c_.get_allocator(); }

        reference top() { return c_.back(); }
        const_reference top() const { return c_.back(); }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void push(const value_type& value) { c_.push_back(value); }
        void push(value_type&& value) { c_.push_back(mystl::move(value)); }

        template <typename... Args>
        decltype(auto) emplace(Args&&... args) 
        { 
            return c_.emplace_back(mystl::forward<Args>(args)...); 
        }

        void pop() { c_.pop_back(); }

        void swap(Stack& other) noexcept(noexcept(mystl::swap(c_, other.c_))) 
        { 
            mystl::swap(c_, other.c_); 
        }
    };

    // Global swap function for ADL support (Argument-Dependent Lookup)
    template <typename T, typename Container>
    void swap(Stack<T, Container>& lhs, Stack<T, Container>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

} // namespace mystl