#pragma once

#include "rb_tree.hpp"
#include "allocator.hpp"
#include "utility.hpp"

#include <initializer_list>
#include <stdexcept>
#include <functional>

namespace mystl 
{
    template <
        typename Key, 
        typename T,
        typename Compare = std::less<Key>, 
        typename Allocator = mystl::Allocator<mystl::Pair<const Key, T>>
    >
    class Map 
    {
    public:
        using key_type               = Key;
        using mapped_type            = T;
        using value_type             = mystl::Pair<const Key, T>;
        
    private:
        // Конфигурируем RBTree: Value = Pair<const Key, T>, экстрактор = Select1st
        using Tree = RBTree<Key, value_type, mystl::Select1st<value_type>, Compare, Allocator>;
        Tree tree_;

    public:
        using size_type              = typename Tree::size_type;
        using difference_type        = typename Tree::difference_type;
        using allocator_type         = Allocator;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        
        using iterator               = typename Tree::iterator;
        using const_iterator         = typename Tree::const_iterator;
        using reverse_iterator       = typename Tree::reverse_iterator;
        using const_reverse_iterator = typename Tree::const_reverse_iterator;

        // ========================================================================
        // КОНСТРУКТОРЫ
        // ========================================================================
        Map() : tree_() {}
        
        explicit Map(const Compare& comp, const Allocator& alloc = Allocator()) 
            : tree_(comp, alloc) 
        {
        }
            
        explicit Map(const Allocator& alloc) 
            : tree_(Compare(), alloc) 
        {
        }

        template <typename InputIt>
        Map(InputIt first, InputIt last, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc) 
        {
            for (; first != last; ++first) 
            {
                tree_.emplace_unique(*first);
            }
        }

        Map(std::initializer_list<value_type> init, const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : tree_(comp, alloc)
        {
            for (const auto& val : init) 
            {
                tree_.emplace_unique(val);
            }
        }

        // ========================================================================
        // RULE OF FIVE И АЛЛОКАТОРЫ
        // ========================================================================
        Map(const Map& other) : tree_(other.tree_) {}
        Map(const Map& other, const Allocator& alloc) : tree_(other.tree_, alloc) {}
        
        Map(Map&& other) noexcept : tree_(mystl::move(other.tree_)) {}
        Map(Map&& other, const Allocator& alloc) : tree_(mystl::move(other.tree_), alloc) {}

        Map& operator=(const Map& other) 
        { 
            tree_ = other.tree_; 
            return *this; 
        }
        
        Map& operator=(Map&& other) noexcept 
        { 
            tree_ = mystl::move(other.tree_); 
            return *this; 
        }

        // ========================================================================
        // ДОСТУП К ЭЛЕМЕНТАМ (ACCESS)
        // ========================================================================
        
        mapped_type& operator[](const key_type& key) 
        {
            // ОПТИМИЗАЦИЯ: Сначала ищем. Если ключ есть — не делаем лишнюю аллокацию памяти!
            iterator it = tree_.find(key);
            if (it != end()) 
                return it->second;
                
            auto res = tree_.emplace_unique(key, T{});
            return res.first->second;
        }

        mapped_type& at(const key_type& key) 
        {
            iterator it = tree_.find(key);
            if (it == end()) 
                throw std::out_of_range("Map::at: key not found");
            return it->second;
        }

        const mapped_type& at(const key_type& key) const 
        {
            const_iterator it = tree_.find(key);
            if (it == cend()) 
                throw std::out_of_range("Map::at: key not found");
            return it->second;
        }

        // ========================================================================
        // СОСТОЯНИЕ КОНТЕЙНЕРА
        // ========================================================================
        [[nodiscard]] allocator_type get_allocator() const noexcept { return tree_.get_allocator(); }
        [[nodiscard]] bool empty() const noexcept { return tree_.empty(); }
        [[nodiscard]] size_type size() const noexcept { return tree_.size(); }
        void clear() noexcept { tree_.clear(); }

        iterator begin() noexcept { return tree_.begin(); }
        iterator end() noexcept { return tree_.end(); }
        const_iterator begin() const noexcept { return tree_.cbegin(); }
        const_iterator end() const noexcept { return tree_.cend(); }
        const_iterator cbegin() const noexcept { return tree_.cbegin(); }
        const_iterator cend() const noexcept { return tree_.cend(); }

        reverse_iterator rbegin() noexcept { return tree_.rbegin(); }
        reverse_iterator rend() noexcept { return tree_.rend(); }
        const_reverse_iterator crbegin() const noexcept { return tree_.crbegin(); }
        const_reverse_iterator crend() const noexcept { return tree_.crend(); }

        // ========================================================================
        // МОДИФИКАТОРЫ (MODIFIERS)
        // ========================================================================
        mystl::Pair<iterator, bool> insert(const value_type& value) 
        {
            return tree_.emplace_unique(value);
        }

        mystl::Pair<iterator, bool> insert(value_type&& value) 
        {
            return tree_.emplace_unique(mystl::move(value));
        }

        template <typename... Args>
        mystl::Pair<iterator, bool> emplace(Args&&... args) 
        {
            return tree_.emplace_unique(mystl::forward<Args>(args)...);
        }

        // C++17 try_emplace
        template <typename... Args>
        mystl::Pair<iterator, bool> try_emplace(const key_type& key, Args&&... args) 
        {
            iterator it = tree_.find(key);
            if (it != end()) 
                return {it, false};
            
            return tree_.emplace_unique(key, T(mystl::forward<Args>(args)...));
        }

        // C++17 insert_or_assign (ИСПРАВЛЕНО И ОПТИМИЗИРОВАНО)
        mystl::Pair<iterator, bool> insert_or_assign(const key_type& key, const T& obj) 
        {
            iterator it = tree_.find(key);
            if (it != end()) 
            {
                it->second = obj; // Если ключ уже есть, безопасно перезаписываем значение
                return {it, false};
            }
            return tree_.emplace_unique(key, obj); // Иначе вставляем новую ноду
        }

        mystl::Pair<iterator, bool> insert_or_assign(const key_type& key, T&& obj) 
        {
            iterator it = tree_.find(key);
            if (it != end()) 
            {
                it->second = mystl::move(obj); // Безопасно перемещаем только если ключ существует!
                return {it, false};
            }
            return tree_.emplace_unique(key, mystl::move(obj)); // Иначе отдаем перемещение внутрь дерева
        }

        size_type erase(const key_type& key) 
        {
            return tree_.erase(key);
        }

        void swap(Map& other) noexcept 
        {
            tree_.swap(other.tree_);
        }

        // ========================================================================
        // ПОИСК (SEARCH)
        // ========================================================================
        iterator find(const key_type& key) { return tree_.find(key); }
        const_iterator find(const key_type& key) const { return tree_.find(key); }

        bool contains(const key_type& key) const { return tree_.contains(key); }

        iterator lower_bound(const key_type& key) { return tree_.lower_bound(key); }
        const_iterator lower_bound(const key_type& key) const { return tree_.lower_bound(key); }

        iterator upper_bound(const key_type& key) { return tree_.upper_bound(key); }
        const_iterator upper_bound(const key_type& key) const { return tree_.upper_bound(key); }
        
        mystl::Pair<iterator, iterator> equal_range(const key_type& key) { return tree_.equal_range(key); }
        mystl::Pair<const_iterator, const_iterator> equal_range(const key_type& key) const { return tree_.equal_range(key); }
    };

} // namespace mystl

// Поддержка allocator traits для Map
namespace std
{
    template <typename Key, typename T, typename Compare, typename Alloc>
    struct uses_allocator<mystl::Map<Key, T, Compare, Alloc>, Alloc> : true_type {};
}