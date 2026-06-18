#pragma once

#include "deque.hpp"
#include "utility.hpp"

#include <memory>
#include <type_traits>

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
        using allocator_type  = typename Container::allocator_type;

        static_assert(std::is_same<T, value_type>::value,
                      "Stack<T, Container>: Container::value_type must be T");

    protected:
        Container c; // Underlying container

    public:
        // Constructors
        Stack() : c() {}
        explicit Stack(const allocator_type& alloc) : c(alloc) {}
        explicit Stack(const Container& cont) : c(cont) {}
        explicit Stack(Container&& cont) : c(mystl::move(cont)) {}
        Stack(const Container& cont, const allocator_type& alloc) : c(cont, alloc) {}
        Stack(Container&& cont, const allocator_type& alloc) : c(mystl::move(cont), alloc) {}
        Stack(const Stack& other, const allocator_type& alloc) : c(other.c, alloc) {}
        Stack(Stack&& other, const allocator_type& alloc) : c(mystl::move(other.c), alloc) {}
        
        Stack(const Stack& other) = default;
        Stack(Stack&& other) noexcept = default;
        Stack& operator=(const Stack& other) = default;
        Stack& operator=(Stack&& other) noexcept = default;
        ~Stack() = default;

        // Element access
        [[nodiscard]] bool empty() const { return c.empty(); }
        [[nodiscard]] size_type size() const { return c.size(); }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return c.get_allocator(); }

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

namespace std
{
    template <typename T, typename Container, typename Alloc>
    struct uses_allocator<mystl::Stack<T, Container>, Alloc>
        : uses_allocator<Container, Alloc>
    {
    };
}
