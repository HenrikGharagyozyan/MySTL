#pragma once

#include "rb_tree.hpp"
#include "allocator.hpp" 
#include "utility.hpp"
#include "functional.hpp"

#include <initializer_list>


namespace mystl 
{
    template <
        typename Key, 
        typename Compare = mystl::less, 
        typename Allocator = mystl::Allocator<Key>
    >
    class Set 
    {
    public:
        using key_type               = Key;
        using value_type             = Key;
        using key_compare            = Compare;
        using value_compare          = Compare;

    private:
        // Configure RBTree: Value = Key, key extractor = Identity
        using Tree = RBTree<Key, Key, mystl::Identity<Key>, Compare, Allocator>;
        Tree tree_;

    public:
        using size_type              = typename Tree::size_type;
        using difference_type        = typename Tree::difference_type;
        using allocator_type         = Allocator;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        using pointer                = typename mystl::allocator_traits<Allocator>::pointer;
        using const_pointer          = typename mystl::allocator_traits<Allocator>::const_pointer;
        
        // In Set the iterator is always constant to prevent breaking the tree balance
        using iterator               = typename Tree::const_iterator;
        using const_iterator         = typename Tree::const_iterator;
        using reverse_iterator       = typename Tree::const_reverse_iterator;
        using const_reverse_iterator = typename Tree::const_reverse_iterator;

        // ========================================================================
        // CONSTRUCTORS
        // ========================================================================
        
        Set() : tree_() {}
        
        explicit Set(const Compare& comp, const Allocator& alloc = Allocator()) 
            : tree_(comp, alloc) 
        {
        }
            
        explicit Set(const Allocator& alloc) 
            : tree_(Compare(), alloc) 
        {
        }

        template <typename InputIt>
        Set(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc) 
        {
            for (; first != last; ++first) 
            {
                tree_.emplace_unique(*first);
            }
        }

        Set(std::initializer_list<value_type> init, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            for (const auto& val : init) 
            {
                tree_.emplace_unique(val);
            }
        }

        // ========================================================================
        // RULE OF FIVE
        // ========================================================================
        
        Set(const Set& other) : tree_(other.tree_) {}
        Set(const Set& other, const Allocator& alloc) : tree_(other.tree_, alloc) {}
        
        Set(Set&& other) noexcept : tree_(mystl::move(other.tree_)) {}
        Set(Set&& other, const Allocator& alloc) : tree_(mystl::move(other.tree_), alloc) {}

        ~Set() = default;

        Set& operator=(const Set& other) 
        { 
            tree_ = other.tree_; 
            return *this; 
        }
        
        Set& operator=(Set&& other) noexcept 
        { 
            tree_ = mystl::move(other.tree_); 
            return *this; 
        }

        // ========================================================================
        // CAPACITY & OBSERVERS
        // ========================================================================
        
        [[nodiscard]] allocator_type get_allocator() const noexcept { return tree_.get_allocator(); }
        [[nodiscard]] bool empty() const noexcept { return tree_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return tree_.size(); }
        
        key_compare key_comp() const { return key_compare(); }
        value_compare value_comp() const { return value_compare(); }

        // ========================================================================
        // ITERATORS
        // ========================================================================
        
        iterator begin() const noexcept { return tree_.cbegin(); }
        iterator end() const noexcept { return tree_.cend(); }
        const_iterator cbegin() const noexcept { return tree_.cbegin(); }
        const_iterator cend() const noexcept { return tree_.cend(); }

        reverse_iterator rbegin() const noexcept { return tree_.crbegin(); }
        reverse_iterator rend() const noexcept { return tree_.crend(); }
        const_reverse_iterator crbegin() const noexcept { return tree_.crbegin(); }
        const_reverse_iterator crend() const noexcept { return tree_.crend(); }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        
        void clear() noexcept { tree_.clear(); }

        mystl::Pair<iterator, bool> insert(const value_type& value) 
        {
            auto res = tree_.emplace_unique(value);
            return { res.first, res.second };
        }

        mystl::Pair<iterator, bool> insert(value_type&& value) 
        {
            auto res = tree_.emplace_unique(mystl::move(value));
            return { res.first, res.second };
        }

        template <typename InputIt>
        void insert(InputIt first, InputIt last) 
        {
            for (; first != last; ++first) {
                tree_.emplace_unique(*first);
            }
        }

        void insert(std::initializer_list<value_type> ilist) 
        {
            insert(ilist.begin(), ilist.end());
        }

        template <typename... Args>
        mystl::Pair<iterator, bool> emplace(Args&&... args) 
        {
            auto res = tree_.emplace_unique(mystl::forward<Args>(args)...);
            return { res.first, res.second };
        }

        size_type erase(const key_type& key) 
        {
            return tree_.erase(key);
        }

        void swap(Set& other) noexcept 
        {
            tree_.swap(other.tree_);
        }

        // ========================================================================
        // SEARCH
        // ========================================================================
        
        iterator find(const key_type& key) const noexcept { return tree_.find(key); }
        bool contains(const key_type& key) const noexcept { return tree_.contains(key); }
        
        iterator lower_bound(const key_type& key) const noexcept { return tree_.lower_bound(key); }
        iterator upper_bound(const key_type& key) const noexcept { return tree_.upper_bound(key); }
        
        mystl::Pair<iterator, iterator> equal_range(const key_type& key) const noexcept 
        { 
            auto res = tree_.equal_range(key);
            return { res.first, res.second };
        }
    };

    // ========================================================================
    // GLOBAL OPERATORS
    // ========================================================================

    template <typename Key, typename Compare, typename Allocator>
    bool operator==(const Set<Key, Compare, Allocator>& lhs, 
                    const Set<Key, Compare, Allocator>& rhs) 
    {
        return lhs.size() == rhs.size() && mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename Key, typename Compare, typename Allocator>
    bool operator!=(const Set<Key, Compare, Allocator>& lhs, 
                    const Set<Key, Compare, Allocator>& rhs) 
    {
        return !(lhs == rhs);
    }

    template <typename Key, typename Compare, typename Allocator>
    void swap(Set<Key, Compare, Allocator>& lhs, 
              Set<Key, Compare, Allocator>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl

// Support for allocator traits for Set
namespace std
{
    template <typename Key, typename Compare, typename Alloc>
    struct uses_allocator<mystl::Set<Key, Compare, Alloc>, Alloc> : true_type {};
}