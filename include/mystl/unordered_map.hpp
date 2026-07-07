#pragma once

#include "hash_table.hpp"
#include "functional.hpp"
#include "allocator.hpp"
#include "utility.hpp"
#include "cstddef.hpp"

#include <stdexcept>


namespace mystl
{
    // Thin façade over the shared HashTable backbone, mirroring how Map wraps
    // RBTree. Unique keys; iterators are mutable but the key half of each value
    // is const (value_type == Pair<const Key, T>).
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
        using Table = HashTable<Key, value_type, Select1st<value_type>, Hash, KeyEqual, Allocator>;
        Table table_;

    public:
        using iterator       = typename Table::iterator;
        using const_iterator = typename Table::const_iterator;

        // ====================================================================
        // ITERATORS
        // ====================================================================
        iterator       begin()  noexcept { return table_.begin(); }
        iterator       end()    noexcept { return table_.end(); }
        const_iterator begin()  const noexcept { return table_.begin(); }
        const_iterator end()    const noexcept { return table_.end(); }
        const_iterator cbegin() const noexcept { return table_.cbegin(); }
        const_iterator cend()   const noexcept { return table_.cend(); }

        // ====================================================================
        // CONSTRUCTORS, RULE OF FIVE
        // ====================================================================
        UnorderedMap() : table_() {}

        explicit UnorderedMap(size_type bucket_count,
                              const hasher&    hf  = hasher(),
                              const key_equal& eql = key_equal())
            : table_(bucket_count, hf, eql)
        {
        }

        UnorderedMap(const UnorderedMap& other) : table_(other.table_) {}
        UnorderedMap(const UnorderedMap& other, const allocator_type& alloc) : table_(other.table_, alloc) {}
        UnorderedMap(UnorderedMap&& other) noexcept : table_(mystl::move(other.table_)) {}
        UnorderedMap(UnorderedMap&& other, const allocator_type& alloc) : table_(mystl::move(other.table_), alloc) {}

        UnorderedMap& operator=(const UnorderedMap& other)     = default;
        UnorderedMap& operator=(UnorderedMap&& other) noexcept = default;

        // ====================================================================
        // CAPACITY & STATE
        // ====================================================================
        [[nodiscard]] bool      empty()        const noexcept { return table_.empty(); }
        [[nodiscard]] size_type size()         const noexcept { return table_.size(); }
        [[nodiscard]] size_type bucket_count() const noexcept { return table_.bucket_count(); }
        [[nodiscard]] float     load_factor()  const noexcept { return table_.load_factor(); }
        [[nodiscard]] float     max_load_factor() const noexcept { return table_.max_load_factor(); }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return table_.get_allocator(); }

        void clear() noexcept { table_.clear(); }

        // ====================================================================
        // SEARCH
        // ====================================================================
        iterator       find(const Key& key)       { return table_.find(key); }
        const_iterator find(const Key& key) const { return table_.find(key); }

        // ====================================================================
        // MODIFIERS
        // ====================================================================
        void rehash(size_type new_count) { table_.rehash(new_count); }

        template <typename... Args>
        mystl::Pair<iterator, bool> emplace(Args&&... args)
        {
            return table_.emplace_unique(mystl::forward<Args>(args)...);
        }

        mystl::Pair<iterator, bool> insert(const value_type& value) { return table_.emplace_unique(value); }
        mystl::Pair<iterator, bool> insert(value_type&& value)      { return table_.emplace_unique(mystl::move(value)); }

        size_type erase(const Key& key) { return table_.erase(key); }

        void swap(UnorderedMap& other) noexcept { table_.swap(other.table_); }

        // ====================================================================
        // ELEMENT ACCESS
        // ====================================================================
        T& operator[](const Key& key)
        {
            iterator it = table_.find(key);
            if (it == table_.end())
                return table_.emplace_unique(key, T()).first->second;
            return it->second;
        }

        T& at(const Key& key)
        {
            iterator it = table_.find(key);
            if (it == table_.end())
                throw std::out_of_range("UnorderedMap::at: key not found");
            return it->second;
        }

        const T& at(const Key& key) const
        {
            const_iterator it = table_.find(key);
            if (it == table_.cend())
                throw std::out_of_range("UnorderedMap::at: key not found");
            return it->second;
        }
    };

} // namespace mystl
