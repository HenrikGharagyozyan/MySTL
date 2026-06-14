#pragma once

#include "deque.hpp"
#include "utility.hpp"

namespace mystl 
{
    // By default, use mystl::Deque, but allow another container to be passed (for example, Vector or List)
    template <typename T, typename Container = mystl::Deque<T>>
    class Stack 
    {
    public:
        using container_type  = Container;
        using value_type      = typename Container::value_type;
        using size_type       = typename Container::size_type;
        using reference       = typename Container::reference;
        using const_reference = typename Container::const_reference;

    protected:
        Container c; // Underlying container

    public:
        // Constructors
        Stack() : c() {}
        explicit Stack(const Container& cont) : c(cont) {}
        explicit Stack(Container&& cont) : c(mystl::move(cont)) {}
        
        Stack(const Stack& other) = default;
        Stack(Stack&& other) noexcept = default;
        Stack& operator=(const Stack& other) = default;
        Stack& operator=(Stack&& other) noexcept = default;
        ~Stack() = default;

        // Element access
        [[nodiscard]] bool empty() const { return c.empty(); }
        [[nodiscard]] size_type size() const { return c.size(); }

        reference top() { return c.back(); }
        const_reference top() const { return c.back(); }

        // Modifiers
        void push(const value_type& value) { c.push_back(value); }
        void push(value_type&& value) { c.push_back(mystl::move(value)); }

        template <typename... Args>
        decltype(auto) emplace(Args&&... args) 
        { 
            return c.emplace_back(mystl::forward<Args>(args)...); 
        }

        void pop() { c.pop_back(); }

        void swap(Stack& other) noexcept(noexcept(mystl::swap(c, other.c))) 
        { 
            mystl::swap(c, other.c); 
        }
    };

    // Global swap function for ADL support
    template <typename T, typename Container>
    void swap(Stack<T, Container>& lhs, Stack<T, Container>& rhs) noexcept(noexcept(lhs.swap(rhs)))
    {
        lhs.swap(rhs);
    }

} // namespace mystl