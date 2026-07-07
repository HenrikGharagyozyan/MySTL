#pragma once

#include "utility.hpp"
#include "functional.hpp"
#include "allocator.hpp"
#include "memory.hpp"
#include "pair.hpp"
#include "cstddef.hpp"


namespace mystl
{
    // ============================================================================
    // HASH TABLE BACKBONE
    //
    // Shared implementation for the unordered associative containers, mirroring
    // the role RBTree plays for the ordered ones. Parameterized by:
    //   - Key         : the lookup key type
    //   - Value        : the stored element (Key for sets, Pair<const Key,T> for maps)
    //   - KeyOfValue   : extracts the Key from a Value (Identity / Select1st)
    //   - Hash, KeyEqual, Allocator
    //
    // Separate chaining with singly-linked nodes. Exposes emplace_unique (returns
    // {iterator, bool}) and emplace_multi (returns iterator, keeps equivalent keys
    // adjacent), exactly as RBTree exposes emplace_unique / emplace_equal.
    // ============================================================================
    template <
        typename Key,
        typename Value,
        typename KeyOfValue,
        typename Hash,
        typename KeyEqual,
        typename Allocator
    >
    class HashTable
    {
    public:
        using key_type        = Key;
        using value_type      = Value;
        using size_type       = typename mystl::allocator_traits<Allocator>::size_type;
        using difference_type = mystl::ptrdiff_t;
        using hasher          = Hash;
        using key_equal       = KeyEqual;
        using allocator_type  = Allocator;
        using reference       = value_type&;
        using const_reference = const value_type&;
        using pointer         = typename mystl::allocator_traits<Allocator>::pointer;
        using const_pointer   = typename mystl::allocator_traits<Allocator>::const_pointer;

    private:
        struct Node
        {
            Value value;
            Node* next;

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
        [[no_unique_address]] KeyOfValue            extract_key_;

        // ====================================================================
        // PRIVATE ALLOCATION & NODE HELPERS
        // ====================================================================
        size_type get_bucket_index(const Key& key, size_type b_count) const
        {
            return hash_func_(key) % b_count;
        }

        Node** allocate_buckets(size_type n)
        {
            Node** p = bucket_traits::allocate(bucket_alloc_, n);
            for (size_type i = 0; i < n; ++i) 
                p[i] = nullptr;
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

        // Deep-copy every chain from other into freshly allocated buckets.
        template <typename SelfLike>
        void copy_chains_from(const SelfLike& other)
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
                        prev->next  = new_node;
                    prev = new_node;
                    curr = curr->next;
                    ++size_;
                }
            }
        }

        // Allocator-extended move: take over the source's storage when our
        // allocator can free it (always-equal, or compares equal), otherwise move
        // the elements into storage owned by our allocator so the source's memory
        // is never released through the wrong allocator. Per-bucket order is
        // preserved, keeping equivalent keys adjacent.
        void adopt_or_move(HashTable& other)
        {
            bool take_ownership;
            if constexpr (node_traits::is_always_equal::value)
                take_ownership = true;
            else
                take_ownership = (node_alloc_ == other.node_alloc_);

            if (take_ownership)
            {
                buckets_ = other.buckets_;
                size_    = other.size_;
                other.buckets_      = nullptr;
                other.bucket_count_ = 0;
                other.size_         = 0;
                return;
            }

            buckets_ = allocate_buckets(bucket_count_);
            try
            {
                for (size_type i = 0; i < other.bucket_count_; ++i)
                {
                    Node* curr = other.buckets_[i];
                    Node* prev = nullptr;
                    while (curr)
                    {
                        Node* new_node = create_node(nullptr, mystl::move(curr->value));
                        if (!prev) 
                            buckets_[i] = new_node;
                        else       
                            prev->next  = new_node;
                        prev = new_node;
                        curr = curr->next;
                        ++size_;
                    }
                }
            }
            catch (...)
            {
                clear();
                deallocate_buckets(buckets_, bucket_count_);
                buckets_ = nullptr;
                throw;
            }
            other.clear();
        }

    public:
        // ====================================================================
        // ITERATORS
        // ====================================================================
        // Single iterator template: mutable and const differ only in NodePtr /
        // Reference / Pointer. Matches the RBTreeIterator / ListIterator design so
        // heterogeneous comparison works in either operand order.
        template <typename NodePtr, typename Ref, typename Ptr>
        class HashIterator
        {
        public:
            using iterator_category = mystl::forward_iterator_tag;
            using value_type        = HashTable::value_type;
            using difference_type   = HashTable::difference_type;
            using pointer           = Ptr;
            using reference         = Ref;

            NodePtr          node_;
            size_type        bucket_idx_;
            const HashTable* table_;

            HashIterator() : node_(nullptr), bucket_idx_(0), table_(nullptr) {}

            HashIterator(NodePtr node, size_type b_idx, const HashTable* table)
                : node_(node), bucket_idx_(b_idx), table_(table)
            {
            }

            // Allow iterator -> const_iterator (never the reverse): the parameter
            // names the mutable instantiation specifically.
            template <typename Dummy = void>
            HashIterator(const HashIterator<Node*, HashTable::reference, HashTable::pointer>& other)
                : node_(other.node_), bucket_idx_(other.bucket_idx_), table_(other.table_)
            {
            }

            reference operator*()  const { return node_->value; }
            pointer   operator->() const { return &(node_->value); }

            HashIterator& operator++()
            {
                node_ = node_->next;
                if (!node_)
                {
                    while (++bucket_idx_ < table_->bucket_count_)
                    {
                        if (table_->buckets_[bucket_idx_])
                        {
                            node_ = table_->buckets_[bucket_idx_];
                            break;
                        }
                    }
                }
                return *this;
            }

            HashIterator operator++(int) { HashIterator tmp = *this; ++(*this); return tmp; }
        };

        using iterator       = HashIterator<Node*, reference, pointer>;
        using const_iterator = HashIterator<const Node*, const_reference, const_pointer>;

        // Heterogeneous, order-independent comparison. Declared as hidden friends of
        // HashTable (an associated class of both iterator kinds) so ADL finds them
        // for every operand combination; a mutable operand converts to const_iterator.
        friend bool operator==(const const_iterator& a, const const_iterator& b) noexcept
        {
            return a.node_ == b.node_;
        }
        friend bool operator!=(const const_iterator& a, const const_iterator& b) noexcept
        {
            return a.node_ != b.node_;
        }

    private:
        iterator make_iterator_safe(Node* node, size_type idx)
        {
            if (node) return iterator(node, idx, this);
            for (size_type i = idx + 1; i < bucket_count_; ++i)
            {
                if (buckets_[i]) 
                    return iterator(buckets_[i], i, this);
            }
            return end();
        }

        const_iterator make_const_iterator_safe(const Node* node, size_type idx) const
        {
            if (node) return const_iterator(node, idx, this);
            for (size_type i = idx + 1; i < bucket_count_; ++i)
            {
                if (buckets_[i]) 
                    return const_iterator(buckets_[i], i, this);
            }
            return cend();
        }

    public:
        iterator begin() noexcept
        {
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                if (buckets_[i]) 
                    return iterator(buckets_[i], i, this);
            }
            return end();
        }
        iterator end() noexcept { return iterator(nullptr, bucket_count_, this); }

        const_iterator begin()  const noexcept { return cbegin(); }
        const_iterator end()    const noexcept { return cend(); }

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

        // ====================================================================
        // CONSTRUCTORS, DESTRUCTOR, RULE OF FIVE
        // ====================================================================
        explicit HashTable(size_type bucket_count = 8,
                           const hasher&    hf  = hasher(),
                           const key_equal& eql = key_equal())
            : bucket_count_(bucket_count)
            , hash_func_(hf)
            , equal_func_(eql)
        {
            buckets_ = allocate_buckets(bucket_count_);
        }

        ~HashTable()
        {
            clear();
            if (buckets_) deallocate_buckets(buckets_, bucket_count_);
        }

        HashTable(const HashTable& other)
            : bucket_count_(other.bucket_count_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(node_traits::select_on_container_copy_construction(other.node_alloc_))
            , bucket_alloc_(bucket_traits::select_on_container_copy_construction(other.bucket_alloc_))
            , hash_func_(other.hash_func_)
            , equal_func_(other.equal_func_)
            , extract_key_(other.extract_key_)
        {
            copy_chains_from(other);
        }

        HashTable(const HashTable& other, const allocator_type& alloc)
            : bucket_count_(other.bucket_count_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(alloc)
            , bucket_alloc_(alloc)
            , hash_func_(other.hash_func_)
            , equal_func_(other.equal_func_)
            , extract_key_(other.extract_key_)
        {
            copy_chains_from(other);
        }

        HashTable(HashTable&& other) noexcept
            : buckets_(other.buckets_)
            , bucket_count_(other.bucket_count_)
            , size_(other.size_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(mystl::move(other.node_alloc_))
            , bucket_alloc_(mystl::move(other.bucket_alloc_))
            , hash_func_(mystl::move(other.hash_func_))
            , equal_func_(mystl::move(other.equal_func_))
            , extract_key_(mystl::move(other.extract_key_))
        {
            other.buckets_      = nullptr;
            other.bucket_count_ = 0;
            other.size_         = 0;
        }

        HashTable(HashTable&& other, const allocator_type& alloc)
            : bucket_count_(other.bucket_count_)
            , max_load_factor_(other.max_load_factor_)
            , node_alloc_(alloc)
            , bucket_alloc_(alloc)
            , hash_func_(mystl::move(other.hash_func_))
            , equal_func_(mystl::move(other.equal_func_))
            , extract_key_(mystl::move(other.extract_key_))
        {
            adopt_or_move(other);
        }

        HashTable& operator=(const HashTable& other)
        {
            if (this != &other)
            {
                HashTable tmp(other);
                swap(tmp);
            }
            return *this;
        }

        HashTable& operator=(HashTable&& other) noexcept
        {
            if (this != &other)
            {
                clear();
                if (buckets_) deallocate_buckets(buckets_, bucket_count_);

                buckets_         = other.buckets_;
                bucket_count_    = other.bucket_count_;
                size_            = other.size_;
                max_load_factor_ = other.max_load_factor_;
                node_alloc_      = mystl::move(other.node_alloc_);
                bucket_alloc_    = mystl::move(other.bucket_alloc_);
                hash_func_       = mystl::move(other.hash_func_);
                equal_func_      = mystl::move(other.equal_func_);
                extract_key_     = mystl::move(other.extract_key_);

                other.buckets_      = nullptr;
                other.bucket_count_ = 0;
                other.size_         = 0;
            }
            return *this;
        }

        // ====================================================================
        // CAPACITY & STATE
        // ====================================================================
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

        // ====================================================================
        // SEARCH
        // ====================================================================
        iterator find(const Key& key)
        {
            if (bucket_count_ == 0) 
                return end();

            size_type idx  = get_bucket_index(key, bucket_count_);
            Node*     curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key))
                    return iterator(curr, idx, this);
                curr = curr->next;
            }
            return end();
        }

        const_iterator find(const Key& key) const
        {
            if (bucket_count_ == 0) 
                return cend();
                
            size_type   idx  = get_bucket_index(key, bucket_count_);
            const Node* curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key))
                    return const_iterator(curr, idx, this);
                curr = curr->next;
            }
            return cend();
        }

        bool contains(const Key& key) const { return find(key) != cend(); }

        size_type count(const Key& key) const
        {
            if (bucket_count_ == 0) return 0;
            size_type   idx  = get_bucket_index(key, bucket_count_);
            const Node* curr = buckets_[idx];
            size_type   cnt  = 0;
            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key)) ++cnt;
                else if (cnt > 0) break; // equivalent keys are adjacent
                curr = curr->next;
            }
            return cnt;
        }

        mystl::Pair<iterator, iterator> equal_range(const Key& key)
        {
            if (bucket_count_ == 0) return { end(), end() };
            size_type idx  = get_bucket_index(key, bucket_count_);
            Node*     curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key))
                {
                    Node* first = curr;
                    while (curr && equal_func_(extract_key_(curr->value), key)) curr = curr->next;
                    return { iterator(first, idx, this), make_iterator_safe(curr, idx) };
                }
                curr = curr->next;
            }
            return { end(), end() };
        }

        mystl::Pair<const_iterator, const_iterator> equal_range(const Key& key) const
        {
            if (bucket_count_ == 0) return { cend(), cend() };
            size_type   idx  = get_bucket_index(key, bucket_count_);
            const Node* curr = buckets_[idx];
            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key))
                {
                    const Node* first = curr;
                    while (curr && equal_func_(extract_key_(curr->value), key)) curr = curr->next;
                    return { const_iterator(first, idx, this), make_const_iterator_safe(curr, idx) };
                }
                curr = curr->next;
            }
            return { cend(), cend() };
        }

        // ====================================================================
        // MODIFIERS
        // ====================================================================
        void rehash(size_type new_count)
        {
            if (new_count <= bucket_count_) return;

            Node** new_buckets = allocate_buckets(new_count);
            for (size_type i = 0; i < bucket_count_; ++i)
            {
                Node* curr = buckets_[i];
                while (curr)
                {
                    Node*     next_node = curr->next;
                    size_type new_idx   = get_bucket_index(extract_key_(curr->value), new_count);
                    curr->next           = new_buckets[new_idx];
                    new_buckets[new_idx] = curr;
                    curr = next_node;
                }
            }
            if (buckets_) deallocate_buckets(buckets_, bucket_count_);
            buckets_      = new_buckets;
            bucket_count_ = new_count;
        }

        // Inserts only if the key is absent. Returns {position, inserted?}.
        template <typename... Args>
        mystl::Pair<iterator, bool> emplace_unique(Args&&... args)
        {
            Node*      new_node = create_node(nullptr, mystl::forward<Args>(args)...);
            const Key& key      = extract_key_(new_node->value);

            iterator it = find(key);
            if (it != end())
            {
                destroy_node(new_node);
                return { it, false };
            }

            if (load_factor() + 1.0f / (bucket_count_ == 0 ? 1 : bucket_count_) > max_load_factor_)
                rehash(bucket_count_ == 0 ? 8 : bucket_count_ * 2);

            size_type idx  = get_bucket_index(key, bucket_count_);
            new_node->next = buckets_[idx];
            buckets_[idx]  = new_node;
            ++size_;
            return { iterator(new_node, idx, this), true };
        }

        // Always inserts, keeping equivalent keys adjacent within the bucket.
        template <typename... Args>
        iterator emplace_multi(Args&&... args)
        {
            if (load_factor() + 1.0f / (bucket_count_ == 0 ? 1 : bucket_count_) > max_load_factor_)
                rehash(bucket_count_ == 0 ? 8 : bucket_count_ * 2);

            Node*      new_node = create_node(nullptr, mystl::forward<Args>(args)...);
            const Key& key      = extract_key_(new_node->value);
            size_type  idx      = get_bucket_index(key, bucket_count_);

            Node* curr     = buckets_[idx];
            bool  inserted = false;

            if (curr && equal_func_(extract_key_(curr->value), key))
            {
                new_node->next = curr->next;
                curr->next     = new_node;
                inserted       = true;
            }
            else
            {
                while (curr && curr->next)
                {
                    if (equal_func_(extract_key_(curr->next->value), key))
                    {
                        new_node->next   = curr->next->next;
                        curr->next->next = new_node;
                        inserted         = true;
                        break;
                    }
                    curr = curr->next;
                }
            }

            if (!inserted)
            {
                new_node->next = buckets_[idx];
                buckets_[idx]  = new_node;
            }

            ++size_;
            return iterator(new_node, idx, this);
        }

        // Removes all elements with the given key (0 or 1 for unique tables).
        size_type erase(const Key& key)
        {
            if (bucket_count_ == 0) return 0;
            size_type idx  = get_bucket_index(key, bucket_count_);
            Node*     curr = buckets_[idx];
            Node*     prev = nullptr;
            size_type erased = 0;

            while (curr)
            {
                if (equal_func_(extract_key_(curr->value), key))
                {
                    Node* to_delete = curr;
                    if (!prev) buckets_[idx] = curr->next;
                    else       prev->next    = curr->next;
                    curr = curr->next;
                    destroy_node(to_delete);
                    --size_;
                    ++erased;
                }
                else
                {
                    if (erased > 0) break; // equivalent keys are adjacent
                    prev = curr;
                    curr = curr->next;
                }
            }
            return erased;
        }

        void swap(HashTable& other) noexcept
        {
            mystl::swap(buckets_,         other.buckets_);
            mystl::swap(bucket_count_,    other.bucket_count_);
            mystl::swap(size_,            other.size_);
            mystl::swap(max_load_factor_, other.max_load_factor_);
            mystl::swap(node_alloc_,      other.node_alloc_);
            mystl::swap(bucket_alloc_,    other.bucket_alloc_);
            mystl::swap(hash_func_,       other.hash_func_);
            mystl::swap(equal_func_,      other.equal_func_);
            mystl::swap(extract_key_,     other.extract_key_);
        }
    };

} // namespace mystl
