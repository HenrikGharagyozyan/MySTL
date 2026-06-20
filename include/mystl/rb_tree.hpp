#pragma once

#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <type_traits>
#include <functional>

#include "utility.hpp"

namespace mystl 
{
    // ============================================================================
    // KEY EXTRACTORS
    // ============================================================================
    
    // For Set: key matches the value
    template <typename T>
    struct Identity 
    {
        const T& operator()(const T& x) const { return x; }
    };

    // For Map: key is the first element of a pair
    template <typename Pair>
    struct Select1st 
    {
        constexpr const auto& operator()(const Pair& p) const noexcept { return p.first; }
    };

    enum class RBColor : bool 
    { 
        Red = false, 
        Black = true 
    };

    // ============================================================================
    // TREE NODE
    // ============================================================================
    template <typename Value>
    struct RBNode 
    {
        RBNode* parent;
        RBNode* left;
        RBNode* right;
        RBColor color;
        Value   value;

        template <typename... Args>
        RBNode(RBNode* p, RBNode* l, RBNode* r, RBColor c, Args&&... args)
            : parent(p)
            , left(l)
            , right(r)
            , color(c)
            , value(mystl::forward<Args>(args)...)
        {
        }
    };

    // ============================================================================
    // ITERATORS
    // ============================================================================
    template <typename Value, typename Pointer, typename Reference>
    class RBTreeIterator 
    {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = Value;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Pointer;
        using reference         = Reference;

        using NodePtr = RBNode<Value>*;

        NodePtr node;
        NodePtr nil; // Pointer to sentinel (dummy node)

        RBTreeIterator() noexcept : node(nullptr), nil(nullptr) {}
        RBTreeIterator(NodePtr n, NodePtr sentinel) noexcept : node(n), nil(sentinel) {}

        // Conversion from non-const to const
        template <typename V, typename P, typename R>
        RBTreeIterator(const RBTreeIterator<V, P, R>& other) noexcept
            : node(other.node)
            , nil(other.nil) 
        {
        }

        reference operator*() const { return node->value; }
        pointer operator->() const { return &(node->value); }

        RBTreeIterator& operator++() 
        {
            if (node->right != nil) 
            {
                node = node->right;
                while (node->left != nil) node = node->left;
            } 
            else 
            {
                NodePtr y = node->parent;
                while (y != nil && node == y->right) 
                {
                    node = y;
                    y = y->parent;
                }
                node = y;
            }
            return *this;
        }

        RBTreeIterator operator++(int) 
        {
            RBTreeIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        RBTreeIterator& operator--() 
        {
            if (node == nil) 
            {
                node = node->parent; // nil parent points to the root of the tree. Finding the maximum.
                while (node != nil && node->right != nil) node = node->right;
            } 
            else if (node->left != nil) 
            {
                node = node->left;
                while (node->right != nil) node = node->right;
            } 
            else 
            {
                NodePtr y = node->parent;
                while (y != nil && node == y->left) 
                {
                    node = y;
                    y = y->parent;
                }
                node = y;
            }
            return *this;
        }

        RBTreeIterator operator--(int) 
        {
            RBTreeIterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const RBTreeIterator& other) const noexcept { return node == other.node; }
        bool operator!=(const RBTreeIterator& other) const noexcept { return node != other.node; }
    };

    // ============================================================================
    // RED-BLACK TREE
    // ============================================================================
    template <
        typename Key,
        typename Value,
        typename KeyOfValue,
        typename Compare,
        typename Allocator
    >
    class RBTree 
    {
    public:
        using key_type               = Key;
        using value_type             = Value;
        using size_type              = std::size_t;
        using difference_type        = std::ptrdiff_t;
        using allocator_type         = Allocator;
        using reference              = value_type&;
        using const_reference        = const value_type&;

        using iterator               = RBTreeIterator<Value, Value*, Value&>;
        using const_iterator         = RBTreeIterator<Value, const Value*, const Value&>;
        using reverse_iterator       = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        using Node = RBNode<Value>;
        using node_allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
        using node_traits = std::allocator_traits<node_allocator_type>;

        [[no_unique_address]] node_allocator_type alloc_;
        [[no_unique_address]] Compare             comp_;
        [[no_unique_address]] KeyOfValue          extract_key_;

        Node*     root_;
        Node*     nil_;
        size_type size_;

        // Node memory management
        Node* allocate_node() { return node_traits::allocate(alloc_, 1); }
        void deallocate_node(Node* p) { node_traits::deallocate(alloc_, p, 1); }

        template <typename... Args>
        Node* create_node(Node* parent, Node* left, Node* right, RBColor color, Args&&... args) 
        {
            Node* node = allocate_node();
            try 
            {
                node_traits::construct(alloc_, node, parent, left, right, color, mystl::forward<Args>(args)...);
            } 
            catch (...) 
            {
                deallocate_node(node);
                throw;
            }
            return node;
        }

        void destroy_node(Node* node) 
        {
            if (node) 
            {
                node_traits::destroy(alloc_, node);
                deallocate_node(node);
            }
        }

        void init_nil()
        {
            nil_ = allocate_node();

            nil_->color = RBColor::Black;

            nil_->parent = nil_;
            nil_->left   = nil_;
            nil_->right  = nil_;

            root_ = nil_;
        }

    public:
        RBTree(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : alloc_(alloc)
            , comp_(comp)
            , extract_key_()
            , root_(nullptr)
            , nil_(nullptr)
            , size_(0) 
        {
            init_nil();
        }

        ~RBTree() 
        {
            clear();
            deallocate_node(nil_); // We don't call destroy_node because value in nil_ was not constructed!
        }

        RBTree(const RBTree& other)
            : alloc_(std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.alloc_))
            , comp_(other.comp_), extract_key_(other.extract_key_), size_(0) 
        {
            init_nil();
            if (other.root_ != other.nil_) 
            {
                root_ = copy_tree(other.root_, nil_, other.nil_);
                size_ = other.size_;
            }
        }

        RBTree(RBTree&& other) noexcept
            : alloc_(std::move(other.alloc_))
            , comp_(std::move(other.comp_))
            , extract_key_(std::move(other.extract_key_))
            , root_(other.root_)
            , nil_(other.nil_)
            , size_(other.size_)
        {
            other.init_nil();
            other.root_ = other.nil_;
            other.size_ = 0;
        }

        RBTree& operator=(const RBTree& other) 
        {
            if (this == &other)
                return *this;

            RBTree temp(other); 
            swap(temp);         
            return *this;
        }

        RBTree& operator=(RBTree&& other) noexcept
        {
            if (this == &other)
                return *this;

            clear();
            deallocate_node(nil_);

            alloc_ = std::move(other.alloc_);
            comp_ = std::move(other.comp_);
            extract_key_ = std::move(other.extract_key_);

            root_ = other.root_;
            nil_ = other.nil_;
            size_ = other.size_;

            other.init_nil();
            other.root_ = other.nil_;
            other.size_ = 0;

            return *this;
        }

        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

        iterator begin() noexcept { return iterator(minimum(root_), nil_); }
        iterator end() noexcept { return iterator(nil_, nil_); }
        const_iterator cbegin() const noexcept { return const_iterator(minimum(root_), nil_); }
        const_iterator cend() const noexcept { return const_iterator(nil_, nil_); }

        // ========================================================================
        // MODIFIERS (INSERT / ERASE)
        // ========================================================================
        
        
        template <typename... Args>
        mystl::Pair<iterator, bool> emplace_unique(Args&&... args) 
        {
            // Create a node immediately to extract the key from it
            Node* z = create_node(nullptr, nil_, nil_, RBColor::Red, mystl::forward<Args>(args)...);
            const Key& key = extract_key_(z->value);

            Node* y = nil_;
            Node* x = root_;

            while (x != nil_) 
            {
                y = x;
                if (comp_(key, extract_key_(x->value))) 
                {
                    x = x->left;
                } 
                else if (comp_(extract_key_(x->value), key)) 
                {
                    x = x->right;
                } 
                else 
                {
                    // Key already exists, delete the created node and return false
                    destroy_node(z);
                    return { iterator(x, nil_), false };
                }
            }

            z->parent = y;
            if (y == nil_) 
                root_ = z;
            else if (comp_(key, extract_key_(y->value))) 
                y->left = z;
            else 
                y->right = z;

            nil_->parent = root_; // nil parent always stores the root of the tree
            insert_fixup(z);
            ++size_;
            return { iterator(z, nil_), true };
        }

        void clear() noexcept
        {
            clear_helper(root_);

            root_ = nil_;
            size_ = 0;

            nil_->parent = nil_;
        }


        // ========================================================================
        // SEARCH
        // ========================================================================
        
        
        iterator find(const Key& key) noexcept 
        {
            Node* current = root_;
            while (current != nil_) 
            {
                if (comp_(key, extract_key_(current->value))) 
                {
                    current = current->left;
                } 
                else if (comp_(extract_key_(current->value), key)) 
                {
                    current = current->right;
                } 
                else 
                {
                    return iterator(current, nil_); // Found
                }
            }
            return end();
        }

        const_iterator find(const Key& key) const noexcept 
        {
            Node* current = root_;
            while (current != nil_) 
            {
                if (comp_(key, extract_key_(current->value))) current = current->left;
                else if (comp_(extract_key_(current->value), key)) current = current->right;
                else return const_iterator(current, nil_);
            }
            return cend();
        }

        bool contains(const Key& key) const noexcept 
        {
            return find(key) != cend();
        }

        iterator lower_bound(const Key& key) noexcept 
        {
            Node* current = root_;
            Node* result = nil_;
            while (current != nil_) 
            {
                if (!comp_(extract_key_(current->value), key)) 
                {
                    result = current; // Current node >= key, remember and go left
                    current = current->left;
                } 
                else 
                {
                    current = current->right; // Current node < key, go right
                }
            }
            return iterator(result, nil_);
        }

        iterator upper_bound(const Key& key) noexcept 
        {
            Node* current = root_;
            Node* result = nil_;
            while (current != nil_) 
            {
                if (comp_(key, extract_key_(current->value))) 
                {
                    result = current; // Current node > key, remember and go left
                    current = current->left;
                } 
                else 
                {
                    current = current->right;
                }
            }
            return iterator(result, nil_);
        }

        // ========================================================================
        // DELETION (ERASE)
        // ========================================================================
        
        
        size_type erase(const Key& key) 
        {
            iterator it = find(key);
            if (it == end()) 
                return 0; // Key not found, nothing to delete
            
            delete_node(it.node);
            return 1;
        }

        // ========================================================================
        // AUXILIARY TOOLS (SWAP, EQUAL_RANGE)
        // ========================================================================
        

        void swap(RBTree& other) noexcept 
        {
            mystl::swap(alloc_, other.alloc_);
            mystl::swap(comp_, other.comp_);
            mystl::swap(extract_key_, other.extract_key_);
            mystl::swap(root_, other.root_);
            mystl::swap(nil_, other.nil_);
            mystl::swap(size_, other.size_);
        }

        mystl::Pair<iterator, iterator> equal_range(const Key& key) noexcept 
        {
            return { lower_bound(key), upper_bound(key) };
        }

        mystl::Pair<const_iterator, const_iterator> equal_range(const Key& key) const noexcept 
        {
            return { lower_bound(key), upper_bound(key) };
        }

    private:
        // ========================================================================
        // AUXILIARY ALGORITHMS
        // ========================================================================
        
        
        Node* minimum(Node* x) const noexcept 
        {
            if (x == nil_) 
                return nil_;
            while (x->left != nil_) 
                x = x->left;
            return x;
        }

        void clear_helper(Node* node) noexcept 
        {
            if (node == nil_) 
                return;
            clear_helper(node->left);
            clear_helper(node->right);
            destroy_node(node);
        }

        Node* copy_tree(Node* source_node, Node* parent_node, Node* source_nil) 
        {
            if (source_node == source_nil) 
                return nil_;
            
            Node* new_node = create_node(parent_node, nil_, nil_, source_node->color, source_node->value);
            
            try 
            {
                new_node->left = copy_tree(source_node->left, new_node, source_nil);
                new_node->right = copy_tree(source_node->right, new_node, source_nil);
            } 
            catch (...)
            {
                clear_helper(new_node->left);
                clear_helper(new_node->right);

                destroy_node(new_node);

                throw;
            }
            return new_node;
        }

        void rotate_left(Node* x) 
        {
            Node* y = x->right;
            x->right = y->left;
            if (y->left != nil_) y->left->parent = x;
            y->parent = x->parent;
            if (x->parent == nil_) root_ = y;
            else if (x == x->parent->left) x->parent->left = y;
            else x->parent->right = y;
            y->left = x;
            x->parent = y;
        }

        void rotate_right(Node* x) 
        {
            Node* y = x->left;
            x->left = y->right;
            if (y->right != nil_) y->right->parent = x;
            y->parent = x->parent;
            if (x->parent == nil_) root_ = y;
            else if (x == x->parent->right) x->parent->right = y;
            else x->parent->left = y;
            y->right = x;
            x->parent = y;
        }

        void insert_fixup(Node* z) 
        {
            while (z->parent->color == RBColor::Red) 
            {
                if (z->parent == z->parent->parent->left) 
                {
                    Node* y = z->parent->parent->right;
                    if (y->color == RBColor::Red) 
                    {
                        z->parent->color = RBColor::Black;
                        y->color = RBColor::Black;
                        z->parent->parent->color = RBColor::Red;
                        z = z->parent->parent;
                    } 
                    else 
                    {
                        if (z == z->parent->right) 
                        {
                            z = z->parent;
                            rotate_left(z);
                        }
                        z->parent->color = RBColor::Black;
                        z->parent->parent->color = RBColor::Red;
                        rotate_right(z->parent->parent);
                    }
                } 
                else 
                {
                    Node* y = z->parent->parent->left;
                    if (y->color == RBColor::Red) 
                    {
                        z->parent->color = RBColor::Black;
                        y->color = RBColor::Black;
                        z->parent->parent->color = RBColor::Red;
                        z = z->parent->parent;
                    } 
                    else 
                    {
                        if (z == z->parent->left) 
                        {
                            z = z->parent;
                            rotate_right(z);
                        }
                        z->parent->color = RBColor::Black;
                        z->parent->parent->color = RBColor::Red;
                        rotate_left(z->parent->parent);
                    }
                }
            }
            root_->color = RBColor::Black;
        }

        // ========================================================================
        // DELETION AND BALANCING LOGIC
        // ========================================================================
        

        void transplant(Node* u, Node* v) 
        {
            if (u->parent == nil_) 
                root_ = v;
            else if (u == u->parent->left) 
                u->parent->left = v;
            else 
                u->parent->right = v;
            
            v->parent = u->parent; 
        }

        void delete_node(Node* z) 
        {
            Node* x;
            Node* y = z;
            RBColor y_original_color = y->color;

            if (z->left == nil_) 
            {
                x = z->right;
                transplant(z, z->right);
            } 
            else if (z->right == nil_) 
            {
                x = z->left;
                transplant(z, z->left);
            } 
            else 
            {
                y = minimum(z->right);
                y_original_color = y->color;
                x = y->right;

                if (y->parent == z) 
                {
                    x->parent = y;
                } 
                else 
                {
                    transplant(y, y->right);
                    y->right = z->right;
                    y->right->parent = y;
                }

                transplant(z, y);
                y->left = z->left;
                y->left->parent = y;
                y->color = z->color;
            }

            destroy_node(z); // Physically destroy the node through the allocator
            --size_;

            if (y_original_color == RBColor::Black) 
            {
                erase_fixup(x);
            }
            
            // Critically important: after reshuffling nodes, update the Sentinel node link
            // This is needed so that the end() iterator can perform the -- operator and jump to the maximum
            if (root_ != nil_) root_->parent = nil_;
            nil_->parent = root_; 
        }

        void erase_fixup(Node* x) 
        {
            while (x != root_ && x->color == RBColor::Black) 
            {
                if (x == x->parent->left) 
                {
                    Node* w = x->parent->right;
                    if (w->color == RBColor::Red) 
                    {
                        w->color = RBColor::Black;
                        x->parent->color = RBColor::Red;
                        rotate_left(x->parent);
                        w = x->parent->right;
                    }
                    if (w->left->color == RBColor::Black && w->right->color == RBColor::Black) 
                    {
                        w->color = RBColor::Red;
                        x = x->parent;
                    } 
                    else 
                    {
                        if (w->right->color == RBColor::Black) 
                        {
                            w->left->color = RBColor::Black;
                            w->color = RBColor::Red;
                            rotate_right(w);
                            w = x->parent->right;
                        }
                        w->color = x->parent->color;
                        x->parent->color = RBColor::Black;
                        w->right->color = RBColor::Black;
                        rotate_left(x->parent);
                        x = root_;
                    }
                } 
                else 
                {
                    Node* w = x->parent->left;
                    if (w->color == RBColor::Red) 
                    {
                        w->color = RBColor::Black;
                        x->parent->color = RBColor::Red;
                        rotate_right(x->parent);
                        w = x->parent->left;
                    }
                    if (w->right->color == RBColor::Black && w->left->color == RBColor::Black) 
                    {
                        w->color = RBColor::Red;
                        x = x->parent;
                    } 
                    else 
                    {
                        if (w->left->color == RBColor::Black) 
                        {
                            w->right->color = RBColor::Black;
                            w->color = RBColor::Red;
                            rotate_left(w);
                            w = x->parent->left;
                        }
                        w->color = x->parent->color;
                        x->parent->color = RBColor::Black;
                        w->left->color = RBColor::Black;
                        rotate_right(x->parent);
                        x = root_;
                    }
                }
            }
            x->color = RBColor::Black;
        }
    };

} // namespace mystl