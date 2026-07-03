#pragma once

#include "hash_table.hpp"
#include "functional.hpp"
#include "allocator.hpp"
#include "utility.hpp"

#include <cstddef>

namespace mystl
{
    // Thin façade over the shared HashTable backbone (multi variant), mirroring
    // how MultiMap wraps RBTree. Equivalent keys are allowed and kept adjacent;
    // iterators are mutable but the key half of each value is const.
    template <
        typename Key,
        typename T,
        typename Hash      = mystl::hash<Key>,
        typename KeyEqual  = mystl::equal_to,
        typename Allocator = mystl::Allocator<mystl::Pair<const Key, T>>
    >
    class UnorderedMultiMap
    {
    public:
        using key_type        = Key;
        using mapped_type     = T;
        using value_type      = mystl::Pair<const Key, T>;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
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
        UnorderedMultiMap() : table_() {}

        explicit UnorderedMultiMap(size_type bucket_count,
                                   const hasher&    hf  = hasher(),
                                   const key_equal& eql = key_equal())
            : table_(bucket_count, hf, eql)
        {
        }

        UnorderedMultiMap(const UnorderedMultiMap& other) : table_(other.table_) {}
        UnorderedMultiMap(const UnorderedMultiMap& other, const allocator_type& alloc) : table_(other.table_, alloc) {}
        UnorderedMultiMap(UnorderedMultiMap&& other) noexcept : table_(mystl::move(other.table_)) {}
        UnorderedMultiMap(UnorderedMultiMap&& other, const allocator_type& alloc) : table_(mystl::move(other.table_), alloc) {}

        UnorderedMultiMap& operator=(const UnorderedMultiMap& other)     = default;
        UnorderedMultiMap& operator=(UnorderedMultiMap&& other) noexcept = default;

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

        size_type count(const Key& key) const { return table_.count(key); }

        mystl::Pair<iterator, iterator> equal_range(const Key& key)
        {
            return table_.equal_range(key);
        }

        mystl::Pair<const_iterator, const_iterator> equal_range(const Key& key) const
        {
            return table_.equal_range(key);
        }

        // ====================================================================
        // MODIFIERS
        // ====================================================================
        void rehash(size_type new_count) { table_.rehash(new_count); }

        template <typename... Args>
        iterator emplace(Args&&... args) { return table_.emplace_multi(mystl::forward<Args>(args)...); }

        iterator insert(const value_type& value) { return table_.emplace_multi(value); }
        iterator insert(value_type&& value)      { return table_.emplace_multi(mystl::move(value)); }

        size_type erase(const Key& key) { return table_.erase(key); }

        void swap(UnorderedMultiMap& other) noexcept { table_.swap(other.table_); }
    };

} // namespace mystl
