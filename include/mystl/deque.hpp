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
    // Define the deque block buffer size in bytes (minimum 512 bytes or 1 element)
    inline constexpr std::size_t deque_buf_size(std::size_t size) 
    {
        return size < 512 ? std::size_t(512 / size) : std::size_t(1);
    }

    // ========================================================================
    // BIDIRECTIONAL DEQUE ITERATOR
    // ========================================================================
    template <typename T, typename Reference, typename Pointer>
    struct DequeIterator 
    {
        using iterator_category = mystl::random_access_iterator_tag;
        using value_type        = T;
        using pointer           = Pointer;
        using reference         = Reference;
        using difference_type   = std::ptrdiff_t;

        using map_pointer = T**;
        using Self        = DequeIterator;

        // Store four pointers for O(1) iteration without division
        T* cur;           // Pointer to the current element
        T* first;         // Pointer to the beginning of the current block
        T* last;          // Pointer to the end of the current block (exclusive)
        map_pointer node; // Pointer to the block-map entry

        static constexpr std::size_t buffer_size() 
        {
            return deque_buf_size(sizeof(T));
        }

        DequeIterator() noexcept : cur(nullptr), first(nullptr), last(nullptr), node(nullptr) {}

        DequeIterator(T* x, map_pointer y) noexcept : cur(x), first(*y), last(*y + buffer_size()), node(y) {}

        // Conversion from non-const to const iterator
        template <typename NonConstRef, typename NonConstPtr>
        DequeIterator(const DequeIterator<T, NonConstRef, NonConstPtr>& other) noexcept
            : cur(other.cur)
            , first(other.first)
            , last(other.last)
            , node(other.node) 
        {
        }

        void set_node(map_pointer new_node) noexcept 
        {
            node = new_node;
            first = *new_node;
            last = first + buffer_size();
        }

        reference operator*() const noexcept { return *cur; }
        pointer operator->() const noexcept { return cur; }

        Self& operator++() noexcept 
        {
            ++cur;
            if (cur == last) 
            { 
                set_node(node + 1);
                cur = first;
            }
            return *this;
        }

        Self operator++(int) noexcept 
        {
            Self tmp = *this;
            ++(*this);
            return tmp;
        }

        Self& operator--() noexcept 
        {
            if (cur == first) 
            { 
                set_node(node - 1);
                cur = last;
            }
            --cur;
            return *this;
        }

        Self operator--(int) noexcept 
        {
            Self tmp = *this;
            --(*this);
            return tmp;
        }

        Self& operator+=(difference_type n) noexcept 
        {
            difference_type offset = n + (cur - first);
            if (offset >= 0 && offset < static_cast<difference_type>(buffer_size())) 
            {
                cur += n;
            } 
            else 
            {
                difference_type node_offset = offset > 0 
                    ? offset / static_cast<difference_type>(buffer_size())
                    : -static_cast<difference_type>((-offset - 1) / buffer_size()) - 1;
                
                set_node(node + node_offset);
                cur = first + (offset - node_offset * static_cast<difference_type>(buffer_size()));
            }
            return *this;
        }

        Self operator+(difference_type n) const noexcept { Self tmp = *this; return tmp += n; }
        Self& operator-=(difference_type n) noexcept { return *this += -n; }
        Self operator-(difference_type n) const noexcept { Self tmp = *this; return tmp -= n; }

        reference operator[](difference_type n) const noexcept { return *(*this + n); }

        difference_type operator-(const Self& x) const noexcept 
        {
            return static_cast<difference_type>(buffer_size()) * (node - x.node - 1) +
                (cur - first) + (x.last - x.cur);
        }

        bool operator==(const Self& x) const noexcept { return cur == x.cur; }
        bool operator!=(const Self& x) const noexcept { return !(*this == x); }
        bool operator<(const Self& x) const noexcept { return node == x.node ? cur < x.cur : node < x.node; }
        bool operator>(const Self& x) const noexcept { return x < *this; }
        bool operator<=(const Self& x) const noexcept { return !(x < *this); }
        bool operator>=(const Self& x) const noexcept { return !(*this < x); }
    };

    // ========================================================================
    // DOUBLE-ENDED QUEUE (DEQUE)
    // ========================================================================
    template <typename T, typename Allocator = mystl::Allocator<T>>
    class Deque 
    {
    public:
        using value_type             = T;
        using allocator_type         = Allocator;
        using allocator_traits_type  = mystl::allocator_traits<Allocator>;
        using size_type              = typename allocator_traits_type::size_type;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = T*;
        using const_pointer          = const T*;
        
        using iterator               = DequeIterator<T, T&, T*>;
        using const_iterator         = DequeIterator<T, const T&, const T*>;
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

    private:
        using map_pointer = T**;
        using map_allocator_type = allocator_traits_type::template rebind_alloc<T*>;
        using map_traits = mystl::allocator_traits<map_allocator_type>;

        iterator    start_;       // Iterator to the first element
        iterator    finish_;      // Iterator to the element after the last
        map_pointer map_ = nullptr;
        size_type   map_size_ = 0;
        
        [[no_unique_address]] Allocator alloc_; // Empty allocator optimization

        static constexpr size_type buffer_size() { return deque_buf_size(sizeof(T)); }

    public:
        // ========================================================================
        // LIFETIME (CONSTRUCTORS / DESTRUCTOR)
        // ========================================================================
        
        Deque() { initialize_map(0); }

        explicit Deque(const Allocator& alloc)
            : alloc_(alloc)
        {
            initialize_map(0);
        }

        explicit Deque(size_type count, const T& value = T(), const Allocator& alloc = Allocator())
            : alloc_(alloc) 
        {
            initialize_map(0);
            try 
            {
                for (size_type i = 0; i < count; ++i)
                {
                    push_back(value);
                }
            } 
            catch (...) 
            {
                release_storage();
                throw;
            }
        }

        Deque(std::initializer_list<T> init, const Allocator& alloc = Allocator())
            : alloc_(alloc) 
        {
            initialize_map(0);
            try 
            {
                for (const auto& item : init) 
                {
                    push_back(item);
                }
            } 
            catch (...) 
            {
                release_storage();
                throw;
            }
        }

        Deque(const Deque& other) 
            : alloc_(allocator_traits_type::select_on_container_copy_construction(other.alloc_)) 
        {
            initialize_map(0);
            try
            {
                for (const auto& value : other)
                {
                    push_back(value);
                }
            }
            catch (...)
            {
                release_storage();
                throw;
            }
        }

        Deque(const Deque& other, const Allocator& alloc)
            : alloc_(alloc)
        {
            initialize_map(0);
            try
            {
                for (const auto& value : other)
                {
                    push_back(value);
                }
            }
            catch (...)
            {
                release_storage();
                throw;
            }
        }

        Deque(Deque&& other) noexcept
            : start_(other.start_)
            , finish_(other.finish_)
            , map_(other.map_)
            , map_size_(other.map_size_)
            , alloc_(mystl::move(other.alloc_)) 
        {
            other.map_ = nullptr;
            other.map_size_ = 0;
            other.start_ = iterator();
            other.finish_ = iterator();
        }

        Deque(Deque&& other, const Allocator& alloc)
            : alloc_(alloc)
        {
            if (alloc_ == other.alloc_)
            {
                start_ = other.start_;
                finish_ = other.finish_;
                map_ = other.map_;
                map_size_ = other.map_size_;

                other.map_ = nullptr;
                other.map_size_ = 0;
                other.start_ = iterator();
                other.finish_ = iterator();
                return;
            }

            initialize_map(0);
            try
            {
                for (auto& value : other)
                {
                    push_back(mystl::move(value));
                }
            }
            catch (...)
            {
                release_storage();
                throw;
            }
        }

        template <typename InputIt>
        Deque(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : alloc_(alloc)
        {
            initialize_map(0);
            try
            {
                for (; first != last; ++first)
                {
                    push_back(*first);
                }
            }
            catch (...)
            {
                release_storage();
                throw;
            }
        }

        ~Deque() 
        {
            release_storage();
        }

        Deque& operator=(const Deque& other) 
        {
            if (this != &other) 
            {
                if (size() >= other.size()) 
                {
                    iterator it = mystl::copy(other.begin(), other.end(), start_);
                    while (finish_ != it) 
                    {
                        pop_back();
                    }
                } 
                else 
                {
                    mystl::copy(other.begin(), other.begin() + size(), start_);
                    for (size_type i = size(); i < other.size(); ++i) 
                    {
                        push_back(other[i]);
                    }
                }
            }
            return *this;
        }

        Deque& operator=(Deque&& other) noexcept 
        {
            if (this != &other) 
            {
                release_storage();
                map_ = other.map_;
                map_size_ = other.map_size_;
                start_ = other.start_;
                finish_ = other.finish_;
                
                other.map_ = nullptr;
                other.map_size_ = 0;
                other.start_ = iterator();
                other.finish_ = iterator();
            }
            return *this;
        }

        // ========================================================================
        // ELEMENT ACCESS AND ITERATORS
        // ========================================================================
        
        [[nodiscard]] iterator begin() noexcept { return start_; }
        [[nodiscard]] iterator end() noexcept { return finish_; }
        [[nodiscard]] const_iterator begin() const noexcept { return start_; }
        [[nodiscard]] const_iterator end() const noexcept { return finish_; }
        [[nodiscard]] const_iterator cbegin() const noexcept { return start_; }
        [[nodiscard]] const_iterator cend() const noexcept { return finish_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        [[nodiscard]] reference operator[](size_type index) noexcept { return start_[static_cast<difference_type>(index)]; }
        [[nodiscard]] const_reference operator[](size_type index) const noexcept { return start_[static_cast<difference_type>(index)]; }

        reference at(size_type index) 
        {
            if (index >= size()) 
                throw std::out_of_range("deque::at index out of range");
            return (*this)[index];
        }
        
        const_reference at(size_type index) const 
        {
            if (index >= size()) 
                throw std::out_of_range("deque::at index out of range");
            return (*this)[index];
        }

        [[nodiscard]] reference front() noexcept { return *start_; }
        [[nodiscard]] const_reference front() const noexcept { return *start_; }
        [[nodiscard]] reference back() noexcept { iterator tmp = finish_; --tmp; return *tmp; }
        [[nodiscard]] const_reference back() const noexcept { const_iterator tmp = finish_; --tmp; return *tmp; }

        [[nodiscard]] size_type size() const noexcept { return map_ == nullptr ? 0 : finish_ - start_; }
        [[nodiscard]] bool empty() const noexcept { return map_ == nullptr || finish_ == start_; }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        
        void push_back(const T& value) 
        {
            if (finish_.cur != finish_.last - 1) 
            {
                allocator_traits_type::construct(alloc_, finish_.cur, value);
                ++finish_;
            } 
            else 
            {
                push_back_aux(value);
            }
        }

        void push_front(const T& value) 
        {
            if (start_.cur != start_.first) 
            {
                --start_;
                allocator_traits_type::construct(alloc_, start_.cur, value);
            } 
            else 
            {
                push_front_aux(value);
            }
        }

        template <typename... Args>
        reference emplace_back(Args&&... args) 
        {
            if (finish_.cur != finish_.last - 1) 
            {
                allocator_traits_type::construct(alloc_, finish_.cur, mystl::forward<Args>(args)...);
                reference ref = *finish_.cur;
                ++finish_;
                return ref;
            }
            return *push_back_aux(mystl::forward<Args>(args)...);
        }

        template <typename... Args>
        reference emplace_front(Args&&... args) 
        {
            if (start_.cur != start_.first) 
            {
                --start_;
                allocator_traits_type::construct(alloc_, start_.cur, mystl::forward<Args>(args)...);
                return *start_.cur;
            }
            return *push_front_aux(mystl::forward<Args>(args)...);
        }

        void pop_back() noexcept 
        {
            if (finish_.cur != finish_.first) 
            {
                --finish_;
                allocator_traits_type::destroy(alloc_, finish_.cur);
            } 
            else 
            {
                --finish_;
                allocator_traits_type::destroy(alloc_, finish_.cur);
                deallocate_block(*(finish_.node + 1)); 
            }
        }

        void pop_front() noexcept 
        {
            allocator_traits_type::destroy(alloc_, start_.cur);
            if (start_.cur != start_.last - 1) 
            {
                ++start_;
            } 
            else 
            {
                deallocate_block(start_.first);
                ++start_;
            }
        }

        void clear() noexcept 
        {
            for (map_pointer node = start_.node + 1; node < finish_.node; ++node) 
            {
                for (T* p = *node; p < *node + buffer_size(); ++p) 
                {
                    allocator_traits_type::destroy(alloc_, p);
                }
                deallocate_block(*node);
            }
            
            if (start_.node != finish_.node) 
            {
                for (T* p = start_.cur; p < start_.last; ++p) allocator_traits_type::destroy(alloc_, p);
                for (T* p = finish_.first; p < finish_.cur; ++p) allocator_traits_type::destroy(alloc_, p);
                deallocate_block(*finish_.node);
            } 
            else 
            {
                for (T* p = start_.cur; p < finish_.cur; ++p) allocator_traits_type::destroy(alloc_, p);
            }
            finish_ = start_;
        }

        void swap(Deque& other) noexcept 
        {
            mystl::swap(start_, other.start_);
            mystl::swap(finish_, other.finish_);
            mystl::swap(map_, other.map_);
            mystl::swap(map_size_, other.map_size_);
            mystl::swap(alloc_, other.alloc_);
        }

    private:
        // ========================================================================
        // AUXILIARY MEMORY MANAGEMENT METHODS
        // ========================================================================
        
        void initialize_map(size_type num_elements) 
        {
            size_type num_nodes = num_elements / buffer_size() + 1;
            map_size_ = mystl::max(size_type(8), num_nodes + 2);
            
            map_allocator_type map_alloc(alloc_);
            map_ = map_traits::allocate(map_alloc, map_size_);
            
            // Place the working block range strictly in the center of the map
            map_pointer nstart = map_ + (map_size_ - num_nodes) / 2;
            map_pointer nfinish = nstart + num_nodes - 1;
            
            try 
            {
                create_nodes(nstart, nfinish);
            } 
            catch (...) 
            {
                map_traits::deallocate(map_alloc, map_, map_size_);
                map_ = nullptr;
                map_size_ = 0;
                throw;
            }
            
            start_.set_node(nstart);
            finish_.set_node(nfinish);
            start_.cur = start_.first;
            finish_.cur = finish_.first + (num_elements % buffer_size());
        }

        void create_nodes(map_pointer nstart, map_pointer nfinish) 
        {
            map_pointer cur;
            try 
            {
                for (cur = nstart; cur <= nfinish; ++cur) 
                {
                    *cur = allocator_traits_type::allocate(alloc_, buffer_size());
                }
            } 
            catch (...) 
            {
                for (map_pointer rollback = nstart; rollback < cur; ++rollback) 
                {
                    allocator_traits_type::deallocate(alloc_, *rollback, buffer_size());
                }
                throw;
            }
        }

        void deallocate_block(T* block_ptr) noexcept 
        {
            if (block_ptr) 
            {
                allocator_traits_type::deallocate(alloc_, block_ptr, buffer_size());
            }
        }

        void release_storage() noexcept
        {
            if (!map_)
                return;

            clear();
            deallocate_block(*start_.node);
            map_allocator_type map_alloc(alloc_);
            map_traits::deallocate(map_alloc, map_, map_size_);

            map_ = nullptr;
            map_size_ = 0;
            start_ = iterator();
            finish_ = iterator();
        }

        template <typename... Args>
        iterator push_back_aux(Args&&... args) 
        {
            reserve_map_at_back();
            *(finish_.node + 1) = allocator_traits_type::allocate(alloc_, buffer_size());
            try 
            {
                allocator_traits_type::construct(alloc_, finish_.cur, mystl::forward<Args>(args)...);
                finish_.set_node(finish_.node + 1);
                finish_.cur = finish_.first;
                return finish_ - 1;
            } 
            catch (...) 
            {
                deallocate_block(*(finish_.node + 1));
                throw;
            }
        }

        template <typename... Args>
        iterator push_front_aux(Args&&... args) 
        {
            reserve_map_at_front();
            *(start_.node - 1) = allocator_traits_type::allocate(alloc_, buffer_size());
            try 
            {
                start_.set_node(start_.node - 1);
                start_.cur = start_.last - 1;
                allocator_traits_type::construct(alloc_, start_.cur, mystl::forward<Args>(args)...);
                return start_;
            } 
            catch (...) 
            {
                ++start_;
                deallocate_block(*(start_.node - 1));
                throw;
            }
        }

        void reserve_map_at_back(size_type nodes_to_add = 1) 
        {
            if (nodes_to_add + 1 > map_size_ - (finish_.node - map_)) 
            {
                reallocate_map(nodes_to_add, false);
            }
        }

        void reserve_map_at_front(size_type nodes_to_add = 1) 
        {
            if (nodes_to_add > static_cast<size_type>(start_.node - map_)) 
            {
                reallocate_map(nodes_to_add, true);
            }
        }

        void reallocate_map(size_type nodes_to_add, bool add_at_front) 
        {
            size_type old_nodes = finish_.node - start_.node + 1;
            size_type new_nodes = old_nodes + nodes_to_add;
            
            size_type new_map_size = map_size_ + mystl::max(map_size_, nodes_to_add) + 2;
            map_allocator_type map_alloc(alloc_);
            map_pointer new_map = map_traits::allocate(map_alloc, new_map_size);
            
            map_pointer new_start = new_map + (new_map_size - new_nodes) / 2 + (add_at_front ? nodes_to_add : 0);
            
            mystl::copy(start_.node, finish_.node + 1, new_start);
            
            map_traits::deallocate(map_alloc, map_, map_size_);
            
            map_ = new_map;
            map_size_ = new_map_size;
            
            start_.set_node(new_start);
            finish_.set_node(new_start + old_nodes - 1);
        }
    };

    // ========================================================================
    // GLOBAL COMPARISON OPERATORS
    // ========================================================================
    template <typename T, typename Alloc>
    bool operator==(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return lhs.size() == rhs.size() && mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename T, typename Alloc>
    bool operator!=(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return !(lhs == rhs);
    }

    template <typename T, typename Alloc>
    bool operator<(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return mystl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename Alloc>
    bool operator>(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return rhs < lhs;
    }

    template <typename T, typename Alloc>
    bool operator<=(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return !(rhs < lhs);
    }

    template <typename T, typename Alloc>
    bool operator>=(const Deque<T, Alloc>& lhs, const Deque<T, Alloc>& rhs) 
    {
        return !(lhs < rhs);
    }

    template <typename T, typename Alloc>
    void swap(Deque<T, Alloc>& lhs, Deque<T, Alloc>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl