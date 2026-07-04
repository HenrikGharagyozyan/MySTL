#pragma once

#include <initializer_list>
#include <stdexcept>

#include "rb_tree.hpp"
#include "allocator.hpp"
#include "utility.hpp"
#include "functional.hpp"
#include "iterator.hpp"
#include "algorithm.hpp"

namespace mystl
{
    template <
        typename Key,
        typename T,
        typename Compare = mystl::less,
        typename Allocator = mystl::Allocator<mystl::Pair<const Key, T>>
    >
    class MultiMap
    {
    public:
        // ========================================================================
        // TYPE ALIASES & NESTED CLASSES
        // ========================================================================
        using key_type               = Key;
        using mapped_type            = T;
        using value_type             = mystl::Pair<const Key, T>;
        using key_compare            = Compare;
        using allocator_type         = Allocator;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        using pointer                = typename mystl::allocator_traits<Allocator>::pointer;
        using const_pointer          = typename mystl::allocator_traits<Allocator>::const_pointer;
        using size_type              = typename mystl::allocator_traits<Allocator>::size_type;
        using difference_type        = std::ptrdiff_t;

    private:
        using Tree = RBTree<Key, value_type, Select1st<value_type>, Compare, Allocator>;
        Tree tree_;

    public:
        class value_compare
        {
            friend class MultiMap;
        protected:
            Compare comp;
            value_compare(Compare c) : comp(c) {}

        public:
            using result_type          = bool;
            using first_argument_type  = value_type;
            using second_argument_type = value_type;

            bool operator()(const value_type& lhs, const value_type& rhs) const
            {
                return comp(lhs.first, rhs.first);
            }
        };

        using iterator               = typename Tree::iterator;
        using const_iterator         = typename Tree::const_iterator;
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

        // ========================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // ========================================================================
        MultiMap() : tree_() {}

        explicit MultiMap(const Compare& comp, const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
        }

        explicit MultiMap(const Allocator& alloc)
            : tree_(Compare(), alloc)
        {
        }

        template <typename InputIt>
        MultiMap(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            insert(first, last);
        }

        MultiMap(std::initializer_list<value_type> ilist, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            insert(ilist.begin(), ilist.end());
        }

        MultiMap(const MultiMap& other) : tree_(other.tree_) {}
        MultiMap(const MultiMap& other, const Allocator& alloc) : tree_(other.tree_, alloc) {}

        MultiMap(MultiMap&& other) noexcept : tree_(mystl::move(other.tree_)) {}
        MultiMap(MultiMap&& other, const Allocator& alloc) noexcept : tree_(mystl::move(other.tree_), alloc) {}

        ~MultiMap() = default;

        MultiMap& operator=(const MultiMap& other)
        {
            tree_ = other.tree_;
            return *this;
        }

        MultiMap& operator=(MultiMap&& other) noexcept
        {
            tree_ = mystl::move(other.tree_);
            return *this;
        }

        MultiMap& operator=(std::initializer_list<value_type> ilist)
        {
            clear();
            insert(ilist.begin(), ilist.end());
            return *this;
        }

        // ========================================================================
        // CAPACITY & OBSERVERS
        // ========================================================================
        [[nodiscard]] bool empty() const noexcept { return tree_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return tree_.size(); }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return tree_.get_allocator(); }
        [[nodiscard]] key_compare key_comp() const { return key_compare(); }
        [[nodiscard]] value_compare value_comp() const { return value_compare(key_compare()); }

        // ========================================================================
        // ITERATORS
        // ========================================================================
        iterator begin() noexcept { return tree_.begin(); }
        iterator end() noexcept { return tree_.end(); }
        const_iterator begin() const noexcept { return tree_.begin(); }
        const_iterator end() const noexcept { return tree_.end(); }
        const_iterator cbegin() const noexcept { return tree_.cbegin(); }
        const_iterator cend() const noexcept { return tree_.cend(); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ========================================================================
        // ELEMENT ACCESS
        // ========================================================================
        mapped_type& at(const key_type& key)
        {
            iterator it = find(key);
            if (it == end())
                throw std::out_of_range("MultiMap::at: key not found");
            return it->second;
        }

        const mapped_type& at(const key_type& key) const
        {
            const_iterator it = find(key);
            if (it == end())
                throw std::out_of_range("MultiMap::at: key not found");
            return it->second;
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void clear() noexcept { tree_.clear(); }

        iterator insert(const value_type& value)
        {
            return tree_.emplace_equal(value);
        }

        iterator insert(value_type&& value)
        {
            return tree_.emplace_equal(mystl::move(value));
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last)
        {
            for (; first != last; ++first)
                tree_.emplace_equal(*first);
        }

        void insert(std::initializer_list<value_type> ilist)
        {
            insert(ilist.begin(), ilist.end());
        }

        template <typename... Args>
        iterator emplace(Args&&... args)
        {
            return tree_.emplace_equal(mystl::forward<Args>(args)...);
        }

        iterator erase(const_iterator pos)
        {
            return tree_.erase(pos);
        }

        // Fixed: `return last` was a const_iterator but return type is iterator (mutable).
        // Construct the correct return type from last's raw node pointers.
        iterator erase(const_iterator first, const_iterator last)
        {
            while (first != last)
                first = erase(first);
            return iterator(last.node, last.nil);
        }

        size_type erase(const key_type& key)
        {
            auto range = tree_.equal_range(key);
            size_type count = 0;
            auto it = range.first;
            while (it != range.second)
            {
                it = tree_.erase(it);
                ++count;
            }
            return count;
        }

        void swap(MultiMap& other) noexcept
        {
            tree_.swap(other.tree_);
        }

        // ========================================================================
        // SEARCH
        // ========================================================================
        iterator find(const key_type& key) noexcept { return tree_.find(key); }
        const_iterator find(const key_type& key) const noexcept { return tree_.find(key); }

        bool contains(const key_type& key) const noexcept { return tree_.contains(key); }

        size_type count(const key_type& key) const noexcept
        {
            auto range = equal_range(key);
            return mystl::distance(range.first, range.second);
        }

        iterator lower_bound(const key_type& key) noexcept { return tree_.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const noexcept { return tree_.lower_bound(key); }

        iterator upper_bound(const key_type& key) noexcept { return tree_.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const noexcept { return tree_.upper_bound(key); }

        mystl::Pair<iterator, iterator> equal_range(const key_type& key) noexcept
        {
            return tree_.equal_range(key);
        }

        mystl::Pair<const_iterator, const_iterator> equal_range(const key_type& key) const noexcept
        {
            return tree_.equal_range(key);
        }
    };

    // ============================================================================
    // NON-MEMBER OPERATORS
    // ============================================================================
    template <typename Key, typename T, typename Compare, typename Allocator>
    bool operator==(const MultiMap<Key, T, Compare, Allocator>& lhs,
                    const MultiMap<Key, T, Compare, Allocator>& rhs)
    {
        return lhs.size() == rhs.size() && mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename Key, typename T, typename Compare, typename Allocator>
    bool operator!=(const MultiMap<Key, T, Compare, Allocator>& lhs,
                    const MultiMap<Key, T, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename Key, typename T, typename Compare, typename Allocator>
    void swap(MultiMap<Key, T, Compare, Allocator>& lhs,
              MultiMap<Key, T, Compare, Allocator>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

} // namespace mystl
