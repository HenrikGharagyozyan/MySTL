#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "memory.hpp"
#include "allocator.hpp"
#include "algorithm.hpp"

#include <cstddef>
#include <initializer_list>

namespace mystl 
{
    namespace detail 
    {
        // Helper structures are moved to detail to avoid depending on iterator constness
        struct ForwardListNodeBase 
        {
            ForwardListNodeBase* next;
            ForwardListNodeBase() noexcept : next(nullptr) {} 
        };

        template <typename T>
        struct ForwardListNode : public ForwardListNodeBase 
        {
            T data;
            
            template <typename... Args>
            explicit ForwardListNode(Args&&... args) 
                : data(mystl::forward<Args>(args)...) 
            {
            }
        };
    }

    // ========================================================================
    // SINGLE TEMPLATE ITERATOR (SINGLY LINKED)
    // ========================================================================
    template <typename Value, typename Pointer, typename Reference>
    class ForwardListIterator 
    {
    public:
        using iterator_category = mystl::forward_iterator_tag;
        using value_type        = Value;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Pointer;
        using reference         = Reference;

        using NodeBase = detail::ForwardListNodeBase;
        using Node     = detail::ForwardListNode<Value>;

        NodeBase* node_ = nullptr;

        ForwardListIterator() noexcept : node_(nullptr) {}
        explicit ForwardListIterator(NodeBase* ptr) noexcept : node_(ptr) {}

        // Conversion constructor from mutable iterator to const iterator
        template <typename P, typename R>
        ForwardListIterator(const ForwardListIterator<Value, P, R>& other) noexcept : node_(other.node_) {}

        reference operator*() const noexcept 
        { 
            return static_cast<Node*>(node_)->data; 
        }

        pointer operator->() const noexcept 
        { 
            return mystl::addressof(static_cast<Node*>(node_)->data); 
        }

        ForwardListIterator& operator++() noexcept 
        { 
            node_ = node_->next; 
            return *this; 
        }

        ForwardListIterator operator++(int) noexcept 
        { 
            ForwardListIterator tmp = *this; 
            ++(*this); 
            return tmp; 
        }

        bool operator==(const ForwardListIterator& rhs) const noexcept { return node_ == rhs.node_; }
        bool operator!=(const ForwardListIterator& rhs) const noexcept { return node_ != rhs.node_; }
    };

    // ========================================================================
    // SINGLY LINKED LIST (FORWARD_LIST)
    // ========================================================================
    template <typename T, typename Allocator = mystl::Allocator<T>>
    class ForwardList 
    {
    private:
        using NodeBase = detail::ForwardListNodeBase;
        using Node     = detail::ForwardListNode<T>;

        // mutable allows taking a non-const pointer &head_ in const methods (cbefore_begin/cend)
        mutable NodeBase head_; 
        
        using allocator_traits_type = mystl::allocator_traits<Allocator>;
        using node_allocator_type   = typename allocator_traits_type::template rebind_alloc<Node>;
        using node_traits           = mystl::allocator_traits<node_allocator_type>;
        
        [[no_unique_address]] node_allocator_type alloc_;

    public:
        using value_type             = T;
        using allocator_type         = Allocator;
        using size_type              = typename allocator_traits_type::size_type;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = T*;
        using const_pointer          = const T*;

        using iterator               = ForwardListIterator<T, T*, T&>;
        using const_iterator         = ForwardListIterator<T, const T*, const T&>;

        // ========================================================================
        // DATA ACCESS & ITERATORS
        // ========================================================================
        iterator before_begin() noexcept { return iterator(&head_); }
        const_iterator before_begin() const noexcept { return const_iterator(&head_); }
        const_iterator cbefore_begin() const noexcept { return const_iterator(&head_); }

        iterator begin() noexcept { return iterator(head_.next); }
        iterator end() noexcept { return iterator(nullptr); }
        const_iterator begin() const noexcept { return const_iterator(head_.next); }
        const_iterator end() const noexcept { return const_iterator(nullptr); }
        const_iterator cbegin() const noexcept { return const_iterator(head_.next); }
        const_iterator cend() const noexcept { return const_iterator(nullptr); }

        bool empty() const noexcept { return head_.next == nullptr; }

        reference front() { return *begin(); }
        const_reference front() const { return *begin(); }

        // ========================================================================
        // CONSTRUCTORS AND RULE OF FIVE
        // ========================================================================
        ForwardList() noexcept : head_() {}

        ~ForwardList() 
        {
            clear();
        }

        ForwardList(const ForwardList& other) : head_()
        {
            NodeBase* current = &head_;
            for (const auto& value : other) 
            {
                current->next = create_node(value);
                current = current->next;
            }
        }

        ForwardList(ForwardList&& other) noexcept
            : head_()
            , alloc_(mystl::move(other.alloc_)) 
        {
            head_.next = other.head_.next;
            other.head_.next = nullptr;
        }

        ForwardList(std::initializer_list<value_type> init) : head_()
        {
            NodeBase* current = &head_;
            for (const auto& value : init) 
            {
                current->next = create_node(value);
                current = current->next;
            }
        }

        ForwardList& operator=(const ForwardList& other)
        {
            if (this != &other) 
            {
                ForwardList temp(other);
                swap(temp);
            }
            return *this;
        }

        ForwardList& operator=(ForwardList&& other) noexcept
        {
            if (this != &other) 
            {
                clear();
                alloc_ = mystl::move(other.alloc_);
                head_.next = other.head_.next;
                other.head_.next = nullptr;
            }
            return *this;
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void swap(ForwardList& other) noexcept 
        { 
            mystl::swap(alloc_, other.alloc_);
            mystl::swap(head_.next, other.head_.next); 
        }

        void clear() noexcept
        {
            NodeBase* current = head_.next;
            while (current) 
            {
                NodeBase* next_node = current->next;
                destroy_node(static_cast<Node*>(current));
                current = next_node;
            }
            head_.next = nullptr;
        }

        template <typename... Args>
        iterator emplace_after(const_iterator pos, Args&&... args)
        {
            Node* new_node = create_node(mystl::forward<Args>(args)...);
            NodeBase* current = pos.node_;
            
            new_node->next = current->next;
            current->next = new_node;
            
            return iterator(new_node);
        }

        iterator insert_after(const_iterator pos, const T& value) { return emplace_after(pos, value); }
        iterator insert_after(const_iterator pos, T&& value) { return emplace_after(pos, mystl::move(value)); }

        iterator erase_after(const_iterator pos)
        {
            NodeBase* current = pos.node_;
            NodeBase* node_to_erase = current->next;
            
            if (node_to_erase) 
            {
                current->next = node_to_erase->next;
                destroy_node(static_cast<Node*>(node_to_erase));
            }
            return iterator(current->next);
        }

        void push_front(const T& value) { insert_after(before_begin(), value); }
        void push_front(T&& value) { insert_after(before_begin(), mystl::move(value)); }

        void pop_front() { erase_after(before_begin()); }

        // ========================================================================
        // LIST MANAGEMENT ALGORITHMS
        // ========================================================================
        void reverse() noexcept
        {
            NodeBase* prev = nullptr;
            NodeBase* current = head_.next;
            
            while (current) 
            {
                NodeBase* next_node = current->next;
                current->next = prev;
                prev = current;
                current = next_node;
            }
            head_.next = prev;
        }

        void unique()
        {
            NodeBase* current = head_.next;
            if (!current) 
                return;

            while (current->next) 
            {
                Node* curr_node = static_cast<Node*>(current);
                Node* next_node = static_cast<Node*>(current->next);

                if (curr_node->data == next_node->data) 
                {
                    NodeBase* duplicate = current->next;
                    current->next = duplicate->next;
                    destroy_node(static_cast<Node*>(duplicate));
                } 
                else 
                {
                    current = current->next;
                }
            }
        }

        void merge(ForwardList& other) noexcept
        {
            if (this == &other) 
                return;

            NodeBase* a = head_.next;
            NodeBase* b = other.head_.next;
            NodeBase* tail = &head_;

            while (a && b) 
            {
                if (static_cast<Node*>(b)->data < static_cast<Node*>(a)->data) 
                {
                    tail->next = b;
                    b = b->next;
                } 
                else 
                {
                    tail->next = a;
                    a = a->next;
                }
                tail = tail->next;
            }
            
            tail->next = a ? a : b;
            other.head_.next = nullptr;
        }

        void sort()
        {
            if (!head_.next || !head_.next->next) 
                return;

            size_type list_size = 1;
            while (true) 
            {
                NodeBase* current = head_.next;
                head_.next = nullptr;
                NodeBase* tail = &head_;
                size_type merges_done = 0;

                while (current) 
                {
                    NodeBase* left = current;
                    NodeBase* right = nullptr;
                    size_type left_size = 0;
                    size_type right_size = 0;

                    for (size_type i = 0; i < list_size && current; ++i) 
                    {
                        ++left_size;
                        current = current->next;
                    }

                    right = current;

                    for (size_type i = 0; i < list_size && current; ++i) 
                    {
                        ++right_size;
                        current = current->next;
                    }

                    while (left_size > 0 || right_size > 0) 
                    {
                        if (left_size == 0) 
                        {
                            tail->next = right; right = right->next; --right_size;
                        } 
                        else if (right_size == 0) 
                        {
                            tail->next = left; left = left->next; --left_size;
                        } 
                        else if (static_cast<Node*>(right)->data < static_cast<Node*>(left)->data) 
                        {
                            tail->next = right; right = right->next; --right_size;
                        } 
                        else 
                        {
                            tail->next = left; left = left->next; --left_size;
                        }
                        tail = tail->next;
                    }
                    tail->next = nullptr;
                    ++merges_done;
                }

                if (merges_done <= 1) break;
                
                list_size *= 2;
            }
        }

    private:
        template <typename... Args>
        Node* create_node(Args&&... args) 
        {
            Node* new_node = node_traits::allocate(alloc_, 1);
            try 
            {
                node_traits::construct(alloc_, new_node, mystl::forward<Args>(args)...);
            } 
            catch (...) 
            {
                node_traits::deallocate(alloc_, new_node, 1);
                throw;
            }
            return new_node;
        }

        void destroy_node(Node* node) noexcept 
        {
            node_traits::destroy(alloc_, node);
            node_traits::deallocate(alloc_, node, 1);
        }
    };

    // ========================================================================
    // GLOBAL OPERATORS AND FUNCTIONS
    // ========================================================================
    template <typename T, typename Allocator>
    inline bool operator==(const ForwardListIterator<T, T*, T&>& lhs, 
                           const ForwardListIterator<T, const T*, const T&>& rhs) noexcept 
    {
        return lhs.node_ == rhs.node_;
    }

    template <typename T, typename Allocator>
    bool operator==(const ForwardList<T, Allocator>& lhs, const ForwardList<T, Allocator>& rhs) 
    {
        auto it1 = lhs.begin();
        auto it2 = rhs.begin();
        while (it1 != lhs.end() && it2 != rhs.end()) 
        {
            if (*it1 != *it2) return false;
            ++it1;
            ++it2;
        }
        return it1 == lhs.end() && it2 == rhs.end();
    }

    template <typename T, typename Allocator>
    bool operator!=(const ForwardList<T, Allocator>& lhs, const ForwardList<T, Allocator>& rhs) 
    {
        return !(lhs == rhs);
    }

    template <typename T, typename Allocator>
    void swap(ForwardList<T, Allocator>& lhs, ForwardList<T, Allocator>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl