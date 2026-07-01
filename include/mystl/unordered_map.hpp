#pragma once

#include "utility.hpp"
#include "functional.hpp"
#include "allocator.hpp"
#include "memory.hpp"
#include <cstddef>
#include <stdexcept>

namespace mystl
{

    template <
        typename Key,
        typename T,
        typename Hash      = mystl::hash<Key>,
        typename KeyEqual  = mystl::equal_to,
        typename Allocator = mystl::Allocator<mystl::Pair<const Key, T>>
    >
    class UnorderedMap
    {
    public:
        using key_type        = Key;
        using mapped_type     = T;
        using value_type      = mystl::Pair<const Key, T>;
        using size_type       = std::size_t;
        using hasher          = Hash;
        using key_equal       = KeyEqual;
        using allocator_type  = Allocator;
        using difference_type = std::ptrdiff_t;
        using pointer         = typename mystl::allocator_traits<Allocator>::pointer;
        using const_pointer   = typename mystl::allocator_traits<Allocator>::const_pointer;
        using reference       = value_type&;
        using const_reference = const value_type&;

    private:
        struct Node
        {
            value_type value;
            Node*      next;

            template <typename... Args>
            Node(Node* n, Args&&... args)
                : value(mystl::forward<Args>(args)...)
                , next(n)
            {
            }
        };

        using node_allocator_type   = typename mystl::allocator_traits<Allocator>::template rebind_alloc<Node>;
        using node_traits           = mystl::allocator_traits<node_allocator_type>;
        using bucket_allocator_type = typename mystl::allocator_traits<Allocator>::template rebind_alloc<Node*>;
        using bucket_traits         = mystl::allocator_traits<bucket_allocator_type>;

        Node**    buckets_         = nullptr;
        size_type bucket_count_    = 0;
        size_type size_            = 0;
        float     max_load_factor_ = 1.0f;

        [[no_unique_address]] node_allocator_type   node_alloc_;
        [[no_unique_address]] bucket_allocator_type bucket_alloc_;
        [[no_unique_address]] hasher                hash_func_;
        [[no_unique_address]] key_equal             equal_func_;

        // ========================================================================
        // PRIVATE ALLOCATION HELPERS
        // ========================================================================
        size_type get_bucket_index(const Key& key, size_type b_count) const
        {
            return hash_func_(key) % b_count;
        }

        Node** allocate_buckets(size_type n)
        {
            Node** p = bucket_traits::allocate(bucket_alloc_, n);
            for (size_type i = 0; i < n; ++i) p[i] = nullptr;
            return p;
        }

        void deallocate_buckets(Node** p, size_type n) noexcept
        {
            bucket_traits::deallocate(bucket_alloc_, p, n);
        }

        template <typename... Args>
        Node* create_node(Node* next, Args&&... args)
        {
            Node* p = node_traits::allocate(node_alloc_, 1);
            try
            {
                node_traits::construct(node_alloc_, p, next, mystl::forward<Args>(args)...);
            }
            catch (...)
            {
                node_traits::deallocate(node_alloc_, p, 1);
                throw;
            }
            return p;
        }

        void destroy_node(Node* p) noexcept
        {
            node_traits::destroy(node_alloc_, p);
            node_traits::deallocate(node_alloc_, p, 1);
        }

    public:
        // ========================================================================
        // ITERATORS
        // ========================================================================
        class Iterator
        {
            friend class UnorderedMap;
        public:
            using iterator_category = mystl::forward_iterator_tag;
            using value_type        = UnorderedMap::value_type;
            using difference_type   = UnorderedMap::difference_type;
            using pointer           = UnorderedMap::pointer;
            using reference         = UnorderedMap::reference;

        private:
            Node*              node_;
            size_type          bucket_idx_;
            const UnorderedMap* map_;

            Iterator(Node* node, size_type b_idx, const UnorderedMap* map)
                : node_(node)
                , bucket_idx_(b_idx)
                , map_(map)
            {
            }

        public:
            Iterator() : node_(nullptr), bucket_idx_(0), map_(nullptr) {}

            reference   operator*()  const { return node_->value; }
            value_type* operator->() const { return &(node_->value); }

            Iterator& operator++()
            {
                node_ = node_->next;
                if (!node_)
                {
                    while (++bucket_idx_ < map_->bucket_count_)
                    {
                        if (map_->buckets_[bucket_idx_])
                        {
                            node_ = map_->buckets_[bucket_idx_];
                            break;
                        }
                    }
                }
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator& other) const { return node_ == other.node_; }
            bool operator!=(const Iterator& other) const { return node_ != other.node_; }
        };

        class ConstIterator
        {
            friend class UnorderedMap;
        public:
            using iterator_category = mystl::forward_iterator_tag;
            using value_type        = UnorderedMap::value_type;
            using difference_type   = UnorderedMap::difference_type;
            using pointer           = UnorderedMap::const_pointer;
            using reference         = UnorderedMap::const_reference;
        
        private:
            const Node*        node_;
            size_type          bucket_idx_;
            const UnorderedMap* map_;

            ConstIterator(const Node* node, size_type b_idx, const UnorderedMap* map)
                : node_(node)
                , bucket_idx_(b_idx)
                , map_(map)
            {
            }

        public:
            ConstIterator() : node_(nullptr), bucket_idx_(0), map_(nullptr) {}

            ConstIterator(const Iterator& it)  // Allow conversion from Iterator to ConstIterator
                : node_(it.node_)
                , bucket_idx_(it.bucket_idx_)
                , map_(it.map_) 
            {
            }  

            const_reference    operator*()  const { return node_->value; }
            const value_type*  operator->() const { return &(node_->value); }

            ConstIterator& operator++()
            {
                node_ = node_->next;
                if (!node_)
                {
                    while (++bucket_idx_ < map_->bucket_count_)
                    {
                        if (map_->buckets_[bucket_idx_])
                        {
                            node_ = map_->buckets_[bucket_idx_];
                            break;
                        }
                    }
                }
                return *this;
            }

            ConstIterator operator++(int) 
            {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const ConstIterator& other) const { return node_ == other.node_; }
            bool operator!=(const ConstIterator& other) const { return node_ != other.node_; }
        };

        using iterator       = Iterator;
        using const_iterator = ConstIterator;

        iterator begin() noexcept
        {
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                if (buckets_[i]) 
                    return iterator(buckets_[i], i, this);
            }
            return end();
        }

        iterator       end()   noexcept { return iterator(nullptr, bucket_count_, this); }
        const_iterator begin() const noexcept { return cbegin(); }
        const_iterator end()   const noexcept { return cend(); }

        const_iterator cbegin() const noexcept
        {
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                if (buckets_[i]) 
                    return const_iterator(buckets_[i], i, this);
            }
            return cend();
        }

        const_iterator cend() const noexcept { return const_iterator(nullptr, bucket_count_, this); }

        // ========================================================================
        // CONSTRUCTORS, DESTRUCTOR, RULE OF FIVE
        // ========================================================================
        UnorderedMap() : UnorderedMap(8) {}

        explicit UnorderedMap(size_type bucket_count,
                              const hasher&    hf  = hasher(),
                              const key_equal& eql = key_equal())
            : bucket_count_(bucket_count)
            , hash_func_(hf)
            , equal_func_(eql)
        {
            buckets_ = allocate_buckets(bucket_count_);
        }

        ~UnorderedMap()
        {
            clear();
            if (buckets_)
                deallocate_buckets(buckets_, bucket_count_);
        }

        UnorderedMap(const UnorderedMap& other)
            : bucket_count_(other.bucket_count_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(node_traits::select_on_container_copy_construction(other.node_alloc_))
            , bucket_alloc_(bucket_traits::select_on_container_copy_construction(other.bucket_alloc_))
            , hash_func_(other.hash_func_)
            , equal_func_(other.equal_func_)
        {
            buckets_ = allocate_buckets(bucket_count_);
            for (size_type i = 0; i < other.bucket_count_; ++i)
            {
                Node* curr = other.buckets_[i];
                Node* prev = nullptr;
                while (curr)
                {
                    Node* new_node = create_node(nullptr, curr->value);
                    if (!prev)
                        buckets_[i] = new_node;
                    else
                        prev->next = new_node;
                    prev = new_node;
                    curr = curr->next;
                    ++size_;
                }
            }
        }

        UnorderedMap(const UnorderedMap& other, const allocator_type& alloc)
            : bucket_count_(other.bucket_count_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(alloc)
            , hash_func_(other.hash_func_)
            , equal_func_(other.equal_func_)
        {
            buckets_ = allocate_buckets(bucket_count_);
            for (size_type i = 0; i < other.bucket_count_; ++i) 
            {
                Node* curr = other.buckets_[i];
                Node* prev = nullptr;
                while (curr) 
                {
                    Node* new_node = create_node(nullptr, curr->value);
                    if (!prev) buckets_[i] = new_node;
                    else prev->next = new_node;
                    prev = new_node;
                    curr = curr->next;
                    size_++;
                }
            }
        }

        UnorderedMap(UnorderedMap&& other) noexcept
            : buckets_(other.buckets_)
            , bucket_count_(other.bucket_count_)
            , size_(other.size_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(mystl::move(other.node_alloc_))
            , bucket_alloc_(mystl::move(other.bucket_alloc_))
            , hash_func_(mystl::move(other.hash_func_))
            , equal_func_(mystl::move(other.equal_func_))
        {
            other.buckets_      = nullptr;
            other.bucket_count_ = 0;
            other.size_         = 0;
        }

        UnorderedMap(UnorderedMap&& other, const allocator_type& alloc) noexcept
            : buckets_(other.buckets_)
            , bucket_count_(other.bucket_count_)
            , size_(other.size_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(alloc)
            , hash_func_(mystl::move(other.hash_func_))
            , equal_func_(mystl::move(other.equal_func_))
        {
            other.buckets_ = nullptr;
            other.bucket_count_ = 0;
            other.size_ = 0;
        }

        UnorderedMap& operator=(const UnorderedMap& other)
        {
            if (this != &other)
            {
                UnorderedMap tmp(other);
                swap(tmp);
            }
            return *this;
        }

        UnorderedMap& operator=(UnorderedMap&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                if (buckets_)
                    deallocate_buckets(buckets_, bucket_count_);

                buckets_         = other.buckets_;
                bucket_count_    = other.bucket_count_;
                size_            = other.size_;
                max_load_factor_ = other.max_load_factor_;
                node_alloc_      = mystl::move(other.node_alloc_);
                bucket_alloc_    = mystl::move(other.bucket_alloc_);
                hash_func_       = mystl::move(other.hash_func_);
                equal_func_      = mystl::move(other.equal_func_);

                other.buckets_      = nullptr;
                other.bucket_count_ = 0;
                other.size_         = 0;
            }
            return *this;
        }

        // ========================================================================
        // SIZE AND STATE
        // ========================================================================
        [[nodiscard]] bool      empty()        const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size()         const noexcept { return size_; }
        [[nodiscard]] size_type bucket_count() const noexcept { return bucket_count_; }

        [[nodiscard]] float load_factor() const noexcept
        {
            return bucket_count_ == 0 ? 0.0f : static_cast<float>(size_) / bucket_count_;
        }

        [[nodiscard]] float          max_load_factor() const noexcept { return max_load_factor_; }
        [[nodiscard]] allocator_type get_allocator()   const noexcept { return allocator_type(node_alloc_); }

        void clear() noexcept
        {
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                Node* curr = buckets_[i];
                while (curr)
                {
                    Node* next = curr->next;
                    destroy_node(curr);
                    curr = next;
                }
                buckets_[i] = nullptr;
            }
            size_ = 0;
        }

        // ========================================================================
        // SEARCH
        // ========================================================================
        iterator find(const Key& key)
        {
            if (bucket_count_ == 0) return end();
            size_type idx  = get_bucket_index(key, bucket_count_);
            Node*     curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(curr->value.first, key))
                    return iterator(curr, idx, this);
                curr = curr->next;
            }
            return end();
        }

        const_iterator find(const Key& key) const
        {
            if (bucket_count_ == 0) return cend();
            size_type   idx  = get_bucket_index(key, bucket_count_);
            const Node* curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(curr->value.first, key))
                    return const_iterator(curr, idx, this);
                curr = curr->next;
            }
            return cend();
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void rehash(size_type new_count)
        {
            if (new_count <= bucket_count_) return;

            Node** new_buckets = allocate_buckets(new_count);

            for (size_type i = 0; i < bucket_count_; ++i)
            {
                Node* curr = buckets_[i];
                while (curr)
                {
                    Node* next_node  = curr->next;
                    size_type new_idx = get_bucket_index(curr->value.first, new_count);
                    curr->next           = new_buckets[new_idx];
                    new_buckets[new_idx] = curr;
                    curr = next_node;
                }
            }

            if (buckets_)
                deallocate_buckets(buckets_, bucket_count_);
            buckets_      = new_buckets;
            bucket_count_ = new_count;
        }

        template <typename... Args>
        mystl::Pair<iterator, bool> emplace(Args&&... args)
        {
            Node* new_node = create_node(nullptr, mystl::forward<Args>(args)...);
            const Key& key = new_node->value.first;

            iterator it = find(key);
            if (it != end())
            {
                destroy_node(new_node);
                return {it, false};
            }

            if (load_factor() + 1.0f / (bucket_count_ == 0 ? 1 : bucket_count_) > max_load_factor_)
                rehash(bucket_count_ == 0 ? 8 : bucket_count_ * 2);

            size_type idx  = get_bucket_index(key, bucket_count_);
            new_node->next = buckets_[idx];
            buckets_[idx]  = new_node;
            ++size_;

            return {iterator(new_node, idx, this), true};
        }

        mystl::Pair<iterator, bool> insert(const value_type& value) { return emplace(value); }
        mystl::Pair<iterator, bool> insert(value_type&& value)      { return emplace(mystl::move(value)); }

        size_type erase(const Key& key)
        {
            if (bucket_count_ == 0) return 0;
            size_type idx  = get_bucket_index(key, bucket_count_);
            Node*     curr = buckets_[idx];
            Node*     prev = nullptr;

            while (curr)
            {
                if (equal_func_(curr->value.first, key))
                {
                    if (!prev)
                        buckets_[idx] = curr->next;
                    else
                        prev->next = curr->next;
                    destroy_node(curr);
                    --size_;
                    return 1;
                }
                prev = curr;
                curr = curr->next;
            }
            return 0;
        }

        void swap(UnorderedMap& other) noexcept
        {
            mystl::swap(buckets_,         other.buckets_);
            mystl::swap(bucket_count_,    other.bucket_count_);
            mystl::swap(size_,            other.size_);
            mystl::swap(max_load_factor_, other.max_load_factor_);
            mystl::swap(node_alloc_,      other.node_alloc_);
            mystl::swap(bucket_alloc_,    other.bucket_alloc_);
            mystl::swap(hash_func_,       other.hash_func_);
            mystl::swap(equal_func_,      other.equal_func_);
        }

        // ========================================================================
        // ACCESS OPERATORS
        // ========================================================================
        T& operator[](const Key& key)
        {
            iterator it = find(key);
            if (it == end())
                return emplace(key, T()).first->second;
            return it->second;
        }

        T& at(const Key& key)
        {
            iterator it = find(key);
            if (it == end())
                throw std::out_of_range("UnorderedMap::at: key not found");
            return it->second;
        }

        const T& at(const Key& key) const
        {
            const_iterator it = find(key);
            if (it == cend())
                throw std::out_of_range("UnorderedMap::at: key not found");
            return it->second;
        }
    };

} // namespace mystl
