#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "memory.hpp"
#include "allocator.hpp"
#include "algorithm.hpp"
#include "cstddef.hpp"

#include <initializer_list>

namespace mystl 
{
    namespace detail 
    {
        // Helper structures are moved to detail to avoid depending on iterator constness
        struct ListNodeBase 
        {
            ListNodeBase* prev;
            ListNodeBase* next;
            ListNodeBase() noexcept : prev(this), next(this) {} 
        };

        template <typename T>
        struct ListNode : public ListNodeBase 
        {
            T data;
            template <typename... Args>
            explicit ListNode(Args&&... args) : data(mystl::forward<Args>(args)...) {}
        };
    }

    // ========================================================================
    // SINGLE TEMPLATE ITERATOR (BIDIRECTIONAL)
    // ========================================================================
    template <typename Value, typename Pointer, typename Reference>
    class ListIterator 
    {
    public:
        using iterator_category = mystl::bidirectional_iterator_tag;
        using value_type        = Value;
        using difference_type   = ptrdiff_t;
        using pointer           = Pointer;
        using reference         = Reference;

        using NodeBase = detail::ListNodeBase;
        using Node     = detail::ListNode<Value>;

        NodeBase* node_ = nullptr;

        ListIterator() noexcept : node_(nullptr) {}
        explicit ListIterator(NodeBase* ptr) noexcept : node_(ptr) {}

        // Conversion constructor from mutable iterator to const iterator
        template <typename P, typename R>
        ListIterator(const ListIterator<Value, P, R>& other) noexcept : node_(other.node_) {}

        reference operator*() const noexcept 
        { 
            return static_cast<Node*>(node_)->data; 
        }

        pointer operator->() const noexcept 
        { 
            return mystl::addressof(static_cast<Node*>(node_)->data); 
        }

        ListIterator& operator++() noexcept 
        { 
            node_ = node_->next; 
            return *this; 
        }

        ListIterator operator++(int) noexcept 
        { 
            ListIterator tmp = *this; 
            ++(*this); 
            return tmp; 
        }

        ListIterator& operator--() noexcept 
        { 
            node_ = node_->prev; 
            return *this; 
        }

        ListIterator operator--(int) noexcept 
        { 
            ListIterator tmp = *this; 
            --(*this); 
            return tmp; 
        }

        bool operator==(const ListIterator& rhs) const noexcept { return node_ == rhs.node_; }
        bool operator!=(const ListIterator& rhs) const noexcept { return node_ != rhs.node_; }
    };

    // ========================================================================
    // DOUBLY LINKED LIST (LIST)
    // ========================================================================
    template <typename T, typename Allocator = mystl::Allocator<T>>
    class List 
    {
    private:
        using NodeBase = detail::ListNodeBase;
        using Node     = detail::ListNode<T>;

        // mutable allows taking a non-const pointer &sentinel_ even in const methods (cbegin/cend)
        // This completely removes the need for ugly const_cast!
        mutable NodeBase sentinel_;
        size_t size_ = 0;
        
        using allocator_traits_type = mystl::allocator_traits<Allocator>;
        using node_allocator_type   = typename allocator_traits_type::template rebind_alloc<Node>;
        using node_traits           = mystl::allocator_traits<node_allocator_type>;
        
        [[no_unique_address]] node_allocator_type alloc_;

    public:
        using allocator_type         = Allocator;
        using value_type             = T;
        using size_type              = mystl::size_t;
        using difference_type        = mystl::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = T*;
        using const_pointer          = const T*;

        using iterator               = ListIterator<T, T*, T&>;
        using const_iterator         = ListIterator<T, const T*, const T&>;
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

        // ========================================================================
        // DATA ACCESS & ITERATORS
        // ========================================================================
        iterator begin() noexcept { return iterator(sentinel_.next); }
        iterator end() noexcept { return iterator(&sentinel_); }
        const_iterator begin() const noexcept { return const_iterator(sentinel_.next); }
        const_iterator end() const noexcept { return const_iterator(&sentinel_); }
        const_iterator cbegin() const noexcept { return const_iterator(sentinel_.next); }
        const_iterator cend() const noexcept { return const_iterator(&sentinel_); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }

        reference front() { return *begin(); }
        reference back() { return *(--end()); }
        const_reference front() const { return *begin(); }
        const_reference back() const { return *(--end()); }

        // ========================================================================
        // CONSTRUCTORS AND DESTRUCTOR
        // ========================================================================
        List() noexcept : sentinel_() {}

        ~List() 
        {
            clear();
        }

        List(const List& other) : sentinel_()
        {
            for (const auto& value : other) 
                push_back(value);
        }

        List(List&& other) noexcept 
            : sentinel_()
            , size_(other.size_)
            , alloc_(mystl::move(other.alloc_)) 
        {
            if (size_ > 0) 
            {
                sentinel_.next = other.sentinel_.next;
                sentinel_.prev = other.sentinel_.prev;
                
                sentinel_.next->prev = &sentinel_;
                sentinel_.prev->next = &sentinel_;
                
                other.sentinel_.next = &other.sentinel_;
                other.sentinel_.prev = &other.sentinel_;
                other.size_ = 0;
            }
        }

        List(std::initializer_list<value_type> init) : sentinel_()
        {
            for (const auto& value : init) 
                push_back(value);
        }

        List& operator=(const List& other) 
        {
            if (this != &other) 
            {
                List temp(other);
                swap(temp);
            }
            return *this;
        }

        List& operator=(List&& other) noexcept 
        {
            if (this != &other) 
            {
                clear();
                swap(other);
            }
            return *this;
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        template <typename... Args>
        iterator emplace(const_iterator pos, Args&&... args) 
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

            // Note: no more const_cast!
            NodeBase* current = pos.node_; 
            NodeBase* prev_node = current->prev;

            link_nodes(prev_node, new_node);
            link_nodes(new_node, current);

            ++size_;
            return iterator(new_node);
        }

        void push_back(const T& value) { emplace(cend(), value); }
        void push_back(T&& value) { emplace(cend(), mystl::move(value)); }

        void push_front(const T& value) { emplace(cbegin(), value); }
        void push_front(T&& value) { emplace(cbegin(), mystl::move(value)); }

        iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
        
        iterator erase(const_iterator pos) 
        {
            NodeBase* node_to_erase = pos.node_;
            NodeBase* next_node = node_to_erase->next;
            
            link_nodes(node_to_erase->prev, node_to_erase->next);
            
            Node* node = static_cast<Node*>(node_to_erase);
            node_traits::destroy(alloc_, node);
            node_traits::deallocate(alloc_, node, 1);
            
            --size_;
            return iterator(next_node);
        }
        
        void pop_back() { if (size_ > 0) erase(--cend()); }
        void pop_front() { if (size_ > 0) erase(cbegin()); }
        
        void clear() noexcept 
        {
            NodeBase* current = sentinel_.next;
            while (current != &sentinel_) 
            {
                NodeBase* next_node = current->next;
                Node* node_to_delete = static_cast<Node*>(current);
                
                node_traits::destroy(alloc_, node_to_delete);
                node_traits::deallocate(alloc_, node_to_delete, 1);
                
                current = next_node;
            }
            
            sentinel_.next = &sentinel_;
            sentinel_.prev = &sentinel_;
            size_ = 0;
        }

        void swap(List& other) noexcept 
        {
            mystl::swap(size_, other.size_);
            mystl::swap(alloc_, other.alloc_);

            mystl::swap(sentinel_.next, other.sentinel_.next);
            mystl::swap(sentinel_.prev, other.sentinel_.prev);

            if (size_ > 0) 
            {
                sentinel_.next->prev = &sentinel_;
                sentinel_.prev->next = &sentinel_;
            } 
            else 
            {
                sentinel_.next = sentinel_.prev = &sentinel_;
            }

            if (other.size_ > 0) 
            {
                other.sentinel_.next->prev = &other.sentinel_;
                other.sentinel_.prev->next = &other.sentinel_;
            } 
            else 
            {
                other.sentinel_.next = other.sentinel_.prev = &other.sentinel_;
            }
        }

        // ========================================================================
        // LIST MANAGEMENT ALGORITHMS
        // ========================================================================
        void reverse() noexcept 
        {
            if (size_ <= 1) 
                return;

            NodeBase* current = &sentinel_;
            do {
                mystl::swap(current->next, current->prev);
                current = current->prev; 
            } while (current != &sentinel_);
        }

        void unique() 
        {
            if (size_ <= 1) 
                return;

            iterator it = begin();
            iterator next_it = it;
            ++next_it;

            while (next_it != end()) 
            {
                if (*it == *next_it) 
                {
                    next_it = erase(next_it);
                } 
                else 
                {
                    it = next_it;
                    ++next_it;
                }
            }
        }

        void merge(List& other) 
        {
            if (this == &other || other.empty()) 
                return;

            iterator it1 = begin();
            iterator it2 = other.begin();

            while (it1 != end() && it2 != other.end()) 
            {
                if (*it2 < *it1) 
                {
                    NodeBase* node_to_move = it2.node_;
                    ++it2;

                    other.link_nodes(node_to_move->prev, node_to_move->next);
                    --other.size_;

                    NodeBase* curr = it1.node_;
                    NodeBase* prev_node = curr->prev;

                    link_nodes(prev_node, node_to_move);
                    link_nodes(node_to_move, curr);
                    ++size_;
                } 
                else 
                {
                    ++it1;
                }
            }

            if (!other.empty()) 
            {
                NodeBase* first_rem = other.sentinel_.next;
                NodeBase* last_rem = other.sentinel_.prev;
                NodeBase* my_last = sentinel_.prev;

                link_nodes(my_last, first_rem);
                link_nodes(last_rem, &sentinel_);

                size_ += other.size_;

                other.sentinel_.next = &other.sentinel_;
                other.sentinel_.prev = &other.sentinel_;
                other.size_ = 0;
            }
        }

        void sort() 
        {
            if (size_ <= 1) 
                return;

            List carry;
            List counter[64];
            int fill = 0;

            while (!empty()) 
            {
                NodeBase* node = sentinel_.next;
                link_nodes(&sentinel_, node->next);
                --size_;

                carry.link_nodes(&carry.sentinel_, node);
                carry.link_nodes(node, &carry.sentinel_);
                carry.size_ = 1;

                int i = 0;
                while (i < fill && !counter[i].empty()) 
                {
                    counter[i].merge(carry);
                    carry.swap(counter[i]);
                    ++i;
                }
                carry.swap(counter[i]);
                if (i == fill) ++fill;
            }

            for (int i = 0; i < fill; ++i) 
            {
                merge(counter[i]);
            }
        }

    private:
        void link_nodes(NodeBase* prev_node, NodeBase* next_node) noexcept 
        {
            prev_node->next = next_node;
            next_node->prev = prev_node;
        }
    };

    // ========================================================================
    // GLOBAL OPERATORS AND FUNCTIONS
    // ========================================================================
    template <typename T, typename Allocator>
    bool operator==(const List<T, Allocator>& lhs, const List<T, Allocator>& rhs) 
    {
        if (lhs.size() != rhs.size()) return false;
        return mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename T, typename Allocator>
    bool operator!=(const List<T, Allocator>& lhs, const List<T, Allocator>& rhs) 
    {
        return !(lhs == rhs);
    }

    template <typename T, typename Allocator>
    void swap(List<T, Allocator>& lhs, List<T, Allocator>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl