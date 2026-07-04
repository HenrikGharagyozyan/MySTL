#pragma once

#include <initializer_list>

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
        typename Compare = mystl::less,
        typename Allocator = mystl::Allocator<Key>
    >
    class MultiSet
    {
    private:
        using Tree = RBTree<Key, Key, Identity<Key>, Compare, Allocator>;
        Tree tree_;

    public:
        // ========================================================================
        // TYPE ALIASES
        // ========================================================================
        using key_type               = Key;
        using value_type             = Key;
        using key_compare            = Compare;
        using value_compare          = Compare;
        using allocator_type         = Allocator;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        using pointer                = typename mystl::allocator_traits<Allocator>::pointer;
        using const_pointer          = typename mystl::allocator_traits<Allocator>::const_pointer;
        using size_type              = typename Tree::size_type;
        using difference_type        = std::ptrdiff_t;

        // Prevent key mutation by forcing all iterators to be const
        using iterator               = typename Tree::const_iterator;
        using const_iterator         = typename Tree::const_iterator;
        using reverse_iterator       = typename Tree::const_reverse_iterator;
        using const_reverse_iterator = typename Tree::const_reverse_iterator;

        // ========================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // ========================================================================
        MultiSet() : tree_() {}

        explicit MultiSet(const Compare& comp, const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
        }

        explicit MultiSet(const Allocator& alloc)
            : tree_(Compare(), alloc)
        {
        }

        template <typename InputIt>
        MultiSet(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            insert(first, last);
        }

        MultiSet(std::initializer_list<value_type> ilist, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            insert(ilist.begin(), ilist.end());
        }

        MultiSet(const MultiSet& other) : tree_(other.tree_) {}

        MultiSet(const MultiSet& other, const Allocator& alloc)
            : tree_(other.tree_, alloc)
        {
        }

        MultiSet(MultiSet&& other) noexcept : tree_(mystl::move(other.tree_)) {}

        MultiSet(MultiSet&& other, const Allocator& alloc) noexcept
            : tree_(mystl::move(other.tree_), alloc)
        {
        }

        ~MultiSet() = default;

        MultiSet& operator=(const MultiSet& other)
        {
            tree_ = other.tree_;
            return *this;
        }

        MultiSet& operator=(MultiSet&& other) noexcept
        {
            tree_ = mystl::move(other.tree_);
            return *this;
        }

        MultiSet& operator=(std::initializer_list<value_type> ilist)
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
        [[nodiscard]] value_compare value_comp() const { return value_compare(); }

        // ========================================================================
        // ITERATORS
        // ========================================================================
        iterator begin() noexcept { return tree_.cbegin(); }
        iterator end() noexcept { return tree_.cend(); }
        const_iterator begin() const noexcept { return tree_.cbegin(); }
        const_iterator end() const noexcept { return tree_.cend(); }
        const_iterator cbegin() const noexcept { return tree_.cbegin(); }
        const_iterator cend() const noexcept { return tree_.cend(); }

        reverse_iterator rbegin() noexcept { return tree_.crbegin(); }
        reverse_iterator rend() noexcept { return tree_.crend(); }
        const_reverse_iterator rbegin() const noexcept { return tree_.crbegin(); }
        const_reverse_iterator rend() const noexcept { return tree_.crend(); }
        const_reverse_iterator crbegin() const noexcept { return tree_.crbegin(); }
        const_reverse_iterator crend() const noexcept { return tree_.crend(); }

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

        iterator erase(const_iterator first, const_iterator last)
        {
            while (first != last)
                first = erase(first);
            return last;
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

        void swap(MultiSet& other) noexcept
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
            return static_cast<const Tree&>(tree_).equal_range(key);
        }

        mystl::Pair<const_iterator, const_iterator> equal_range(const key_type& key) const noexcept
        {
            return tree_.equal_range(key);
        }
    };

    // ============================================================================
    // NON-MEMBER OPERATORS
    // ============================================================================
    template <typename Key, typename Compare, typename Allocator>
    bool operator==(const MultiSet<Key, Compare, Allocator>& lhs,
                    const MultiSet<Key, Compare, Allocator>& rhs)
    {
        return lhs.size() == rhs.size() && mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename Key, typename Compare, typename Allocator>
    bool operator!=(const MultiSet<Key, Compare, Allocator>& lhs,
                    const MultiSet<Key, Compare, Allocator>& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename Key, typename Compare, typename Allocator>
    void swap(MultiSet<Key, Compare, Allocator>& lhs,
              MultiSet<Key, Compare, Allocator>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

} // namespace mystl
