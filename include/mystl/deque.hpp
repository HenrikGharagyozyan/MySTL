#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <cassert>

#include "allocator.hpp"
#include "utility.hpp"

namespace mystl 
{

    inline constexpr std::size_t deque_buf_size(std::size_t size) 
    {
        return size < 512 ? std::size_t(512 / size) : std::size_t(1);
    }

    template <typename T, typename Reference, typename Pointer>
    struct DequeIterator 
    {
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = T;
        using pointer           = Pointer;
        using reference         = Reference;
        using difference_type   = std::ptrdiff_t;

        using map_pointer = T**;
        using Self        = DequeIterator;

        // Key optimization: 4 pointers for O(1) iteration without division
        T* cur;       // Points to the current element
        T* first;     // Points to the beginning of the current block
        T* last;      // Points to the end of the current block (exclusive)
        map_pointer node; // Points to the block map itself

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
            if (cur == last) // Reached the end of the block, jump to the next one
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
            if (cur == first) // Reached the beginning of the block, jump to the previous one
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
                // Stay in the same block
                cur += n;
            } 
            else 
            {
                // Jump across blocks
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

    template <typename T, typename Allocator = mystl::Allocator<T>>
    class Deque 
    {
    public:
        using value_type             = T;
        using allocator_type         = Allocator;
        using allocator_traits_type  = std::allocator_traits<Allocator>;
        using size_type              = typename allocator_traits_type::size_type;
        using difference_type        = typename allocator_traits_type::difference_type;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = T*;
        using const_pointer          = const T*;
        
        using iterator               = DequeIterator<T, T&, T*>;
        using const_iterator         = DequeIterator<T, const T&, const T*>;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        using map_pointer = T**;
        using map_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<T*>;

        iterator    start_;       // Iterator to the first element
        iterator    finish_;      // Iterator to the element after the last one
        map_pointer map_ = nullptr;
        size_type   map_size_ = 0;
        
        [[no_unique_address]] Allocator alloc_; // Empty allocator optimization (C++20)

        static constexpr size_type buffer_size() { return deque_buf_size(sizeof(T)); }

    public:
        // ========================================================================
        // LIFE CYCLE (Constructors / Destructor)
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
            : alloc_(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc_)) 
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
                    iterator it = std::copy(other.begin(), other.end(), start_);
                    while (finish_ != it) 
                    {
                        pop_back();
                    }
                } 
                else 
                {
                    std::copy(other.begin(), other.begin() + size(), start_);
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

        [[nodiscard]] reference front() noexcept { assert(!empty()); return *start_; }
        [[nodiscard]] const_reference front() const noexcept { assert(!empty()); return *start_; }
        [[nodiscard]] reference back() noexcept { assert(!empty()); iterator tmp = finish_; --tmp; return *tmp; }
        [[nodiscard]] const_reference back() const noexcept { assert(!empty()); const_iterator tmp = finish_; --tmp; return *tmp; }

        [[nodiscard]] size_type size() const noexcept { return map_ == nullptr ? 0 : finish_ - start_; }
        [[nodiscard]] bool empty() const noexcept { return map_ == nullptr || finish_ == start_; }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

        // ========================================================================
        // MODIFIERS (Push / Pop / Emplace)
        // ========================================================================
        
        void push_back(const T& value) 
        {
            if (finish_.cur != finish_.last - 1) 
            {
                std::allocator_traits<Allocator>::construct(alloc_, finish_.cur, value);
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
                std::allocator_traits<Allocator>::construct(alloc_, start_.cur, value);
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
                std::allocator_traits<Allocator>::construct(alloc_, finish_.cur, mystl::forward<Args>(args)...);
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
                std::allocator_traits<Allocator>::construct(alloc_, start_.cur, mystl::forward<Args>(args)...);
                return *start_.cur;
            }
            return *push_front_aux(mystl::forward<Args>(args)...);
        }

        void pop_back() noexcept 
        {
            assert(!empty());
            if (finish_.cur != finish_.first) 
            {
                --finish_;
                std::allocator_traits<Allocator>::destroy(alloc_, finish_.cur);
            } 
            else 
            {
                --finish_;
                std::allocator_traits<Allocator>::destroy(alloc_, finish_.cur);
                deallocate_block(*(finish_.node + 1)); 
            }
        }

        void pop_front() noexcept 
        {
            assert(!empty());
            std::allocator_traits<Allocator>::destroy(alloc_, start_.cur);
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
                    std::allocator_traits<Allocator>::destroy(alloc_, p);
                }
                deallocate_block(*node);
            }
            
            if (start_.node != finish_.node) 
            {
                for (T* p = start_.cur; p < start_.last; ++p) std::allocator_traits<Allocator>::destroy(alloc_, p);
                for (T* p = finish_.first; p < finish_.cur; ++p) std::allocator_traits<Allocator>::destroy(alloc_, p);
                deallocate_block(*finish_.node);
            } 
            else 
            {
                for (T* p = start_.cur; p < finish_.cur; ++p) std::allocator_traits<Allocator>::destroy(alloc_, p);
            }
            finish_ = start_;
        }

    private:
        // ========================================================================
        // MEMORY MANAGEMENT HELPERS
        // ========================================================================
        
        void initialize_map(size_type num_elements) 
        {
            size_type num_nodes = num_elements / buffer_size() + 1;
            map_size_ = std::max(size_type(8), num_nodes + 2);
            
            map_allocator_type map_alloc(alloc_);
            map_ = std::allocator_traits<map_allocator_type>::allocate(map_alloc, map_size_);
            
            // Place the working range of blocks strictly in the center of the map
            map_pointer nstart = map_ + (map_size_ - num_nodes) / 2;
            map_pointer nfinish = nstart + num_nodes - 1;
            
            try 
            {
                create_nodes(nstart, nfinish);
            } 
            catch (...) 
            {
                std::allocator_traits<map_allocator_type>::deallocate(map_alloc, map_, map_size_);
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
                    *cur = std::allocator_traits<Allocator>::allocate(alloc_, buffer_size());
                }
            } 
            catch (...) 
            {
                for (map_pointer rollback = nstart; rollback < cur; ++rollback) 
                {
                    std::allocator_traits<Allocator>::deallocate(alloc_, *rollback, buffer_size());
                }
                throw;
            }
        }

        void deallocate_block(T* block_ptr) noexcept 
        {
            if (block_ptr) 
            {
                std::allocator_traits<Allocator>::deallocate(alloc_, block_ptr, buffer_size());
            }
        }

        void destroy_map() noexcept 
        {
            if (!map_) 
                return;
            map_allocator_type map_alloc(alloc_);
            std::allocator_traits<map_allocator_type>::deallocate(map_alloc, map_, map_size_);
            map_ = nullptr;
            map_size_ = 0;
        }

        void release_storage() noexcept
        {
            if (!map_)
                return;

            clear();
            deallocate_block(*start_.node);
            map_allocator_type map_alloc(alloc_);
            std::allocator_traits<map_allocator_type>::deallocate(map_alloc, map_, map_size_);

            map_ = nullptr;
            map_size_ = 0;
            start_ = iterator();
            finish_ = iterator();
        }

        template <typename... Args>
        iterator push_back_aux(Args&&... args) 
        {
            reserve_map_at_back();
            *(finish_.node + 1) = std::allocator_traits<Allocator>::allocate(alloc_, buffer_size());
            try 
            {
                std::allocator_traits<Allocator>::construct(alloc_, finish_.cur, mystl::forward<Args>(args)...);
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
            *(start_.node - 1) = std::allocator_traits<Allocator>::allocate(alloc_, buffer_size());
            try 
            {
                start_.set_node(start_.node - 1);
                start_.cur = start_.last - 1;
                std::allocator_traits<Allocator>::construct(alloc_, start_.cur, mystl::forward<Args>(args)...);
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
            
            size_type new_map_size = map_size_ + std::max(map_size_, nodes_to_add) + 2;
            map_allocator_type map_alloc(alloc_);
            map_pointer new_map = std::allocator_traits<map_allocator_type>::allocate(map_alloc, new_map_size);
            
            map_pointer new_start = new_map + (new_map_size - new_nodes) / 2 + (add_at_front ? nodes_to_add : 0);
            
            std::copy(start_.node, finish_.node + 1, new_start);
            
            std::allocator_traits<map_allocator_type>::deallocate(map_alloc, map_, map_size_);
            
            map_ = new_map;
            map_size_ = new_map_size;
            
            start_.set_node(new_start);
            finish_.set_node(new_start + old_nodes - 1);
        }
    };

} // namespace mystl
