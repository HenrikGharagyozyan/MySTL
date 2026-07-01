#pragma once

#include <cstddef>

#include "allocator.hpp"
#include "memory.hpp"
#include "iterator.hpp"
#include "algorithm.hpp"
#include "utility.hpp"
#include "functional.hpp"

namespace mystl 
{
    // ============================================================================
    // KEY EXTRACTORS
    // ============================================================================
    template <typename T>
    struct Identity 
    {
        const T& operator()(const T& x) const { return x; }
    };

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
    // NON-TEMPLATE NODE
    // ============================================================================

    struct RBNodeBase 
    {
        RBNodeBase* parent;
        RBNodeBase* left;
        RBNodeBase* right;
        RBColor color;

        RBNodeBase() = default;

        RBNodeBase(RBNodeBase* p, RBNodeBase* l, RBNodeBase* r, RBColor c)
            : parent(p), left(l), right(r), color(c) 
        {
        }
    };

    template <typename Value>
    struct RBNode : public RBNodeBase
    {
        Value value;

        template <typename... Args>
        RBNode(RBNodeBase* p, RBNodeBase* l, RBNodeBase* r, RBColor c, Args&&... args)
            : RBNodeBase(p, l, r, c), value(mystl::forward<Args>(args)...) 
        {
        }
    };

    // ============================================================================
    // ALGORITHMS 
    // ============================================================================

    struct RBTreeAlgorithms 
    {
        static RBNodeBase* minimum(RBNodeBase* node, RBNodeBase* nil) noexcept 
        {
            if (node == nil) return nil; 
            while (node->left != nil) node = node->left;
            return node;
        }

        static RBNodeBase* maximum(RBNodeBase* node, RBNodeBase* nil) noexcept 
        {
            if (node == nil) return nil;
            while (node->right != nil) node = node->right;
            return node;
        }

        static void rotate_left(RBNodeBase* node, RBNodeBase*& root, RBNodeBase* nil) noexcept
        {
            RBNodeBase* y = node->right;
            node->right = y->left;
            if (y->left != nil) y->left->parent = node;
            y->parent = node->parent;
            if (node->parent == nil) root = y;
            else if (node == node->parent->left) node->parent->left = y;
            else node->parent->right = y;
            y->left = node;
            node->parent = y;
        }

        static void rotate_right(RBNodeBase* node, RBNodeBase*& root, RBNodeBase* nil) noexcept
        {
            RBNodeBase* y = node->left;
            node->left = y->right;
            if (y->right != nil) y->right->parent = node;
            y->parent = node->parent;
            if (node->parent == nil) root = y;
            else if (node == node->parent->right) node->parent->right = y;
            else node->parent->left = y;
            y->right = node;
            node->parent = y;
        }

        static void insert_fixup(RBNodeBase* node, RBNodeBase*& root, RBNodeBase* nil) noexcept
        {
            while (node->parent->color == RBColor::Red) 
            {
                if (node->parent == node->parent->parent->left) 
                {
                    RBNodeBase* y = node->parent->parent->right;
                    if (y->color == RBColor::Red) 
                    {
                        node->parent->color = RBColor::Black;
                        y->color = RBColor::Black;
                        node->parent->parent->color = RBColor::Red;
                        node = node->parent->parent;
                    } 
                    else 
                    {
                        if (node == node->parent->right) 
                        {
                            node = node->parent;
                            rotate_left(node, root, nil);
                        }
                        node->parent->color = RBColor::Black;
                        node->parent->parent->color = RBColor::Red;
                        rotate_right(node->parent->parent, root, nil);
                    }
                } 
                else 
                {
                    RBNodeBase* y = node->parent->parent->left;
                    if (y->color == RBColor::Red) 
                    {
                        node->parent->color = RBColor::Black;
                        y->color = RBColor::Black;
                        node->parent->parent->color = RBColor::Red;
                        node = node->parent->parent;
                    } 
                    else 
                    {
                        if (node == node->parent->left) 
                        {
                            node = node->parent;
                            rotate_right(node, root, nil);
                        }
                        node->parent->color = RBColor::Black;
                        node->parent->parent->color = RBColor::Red;
                        rotate_left(node->parent->parent, root, nil);
                    }
                }
            }
            root->color = RBColor::Black;
        }

        static void transplant(RBNodeBase* u, RBNodeBase* v, RBNodeBase*& root, RBNodeBase* nil) noexcept 
        {
            if (u->parent == nil) root = v;
            else if (u == u->parent->left) u->parent->left = v;
            else u->parent->right = v;
            v->parent = u->parent; 
        }

        static void erase_fixup(RBNodeBase* x, RBNodeBase*& root, RBNodeBase* nil) noexcept 
        {
            while (x != root && x->color == RBColor::Black) 
            {
                if (x == x->parent->left) 
                {
                    RBNodeBase* w = x->parent->right;
                    if (w->color == RBColor::Red) 
                    {
                        w->color = RBColor::Black;
                        x->parent->color = RBColor::Red;
                        rotate_left(x->parent, root, nil);
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
                            rotate_right(w, root, nil);
                            w = x->parent->right;
                        }
                        w->color = x->parent->color;
                        x->parent->color = RBColor::Black;
                        w->right->color = RBColor::Black;
                        rotate_left(x->parent, root, nil);
                        x = root;
                    }
                } 
                else 
                {
                    RBNodeBase* w = x->parent->left;
                    if (w->color == RBColor::Red) 
                    {
                        w->color = RBColor::Black;
                        x->parent->color = RBColor::Red;
                        rotate_right(x->parent, root, nil);
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
                            rotate_left(w, root, nil);
                            w = x->parent->left;
                        }
                        w->color = x->parent->color;
                        x->parent->color = RBColor::Black;
                        w->left->color = RBColor::Black;
                        rotate_right(x->parent, root, nil);
                        x = root;
                    }
                }
            }
            x->color = RBColor::Black;
        }
    }; 


    // ============================================================================
    // ITERATORS
    // ============================================================================

    template <typename Value, typename Pointer, typename Reference>
    class RBTreeIterator 
    {
    public:
        using iterator_category = mystl::bidirectional_iterator_tag;
        using value_type        = Value;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Pointer;
        using reference         = Reference;

        using BasePtr = RBNodeBase*;

        BasePtr node;
        BasePtr nil;

        RBTreeIterator() noexcept : node(nullptr), nil(nullptr) {}
        RBTreeIterator(BasePtr n, BasePtr sentinel) noexcept : node(n), nil(sentinel) {}

        RBTreeIterator(const RBTreeIterator&) = default;
        RBTreeIterator& operator=(const RBTreeIterator&) = default;
        RBTreeIterator(RBTreeIterator&&) = default;
        RBTreeIterator& operator=(RBTreeIterator&&) = default;

        // Allow conversion iterator -> const_iterator, but not vice versa
        template <typename Dummy = void>
        RBTreeIterator(const RBTreeIterator<Value, Value*, Value&>& other) noexcept
            : node(other.node), nil(other.nil) 
        {
        }

        reference operator*() const { return static_cast<RBNode<Value>*>(node)->value; }
        pointer operator->() const { return &(static_cast<RBNode<Value>*>(node)->value); }

        RBTreeIterator& operator++() 
        {
            if (node->right != nil) 
            {
                node = node->right;
                while (node->left != nil) node = node->left;
            } 
            else 
            {
                BasePtr y = node->parent;
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
                node = node->parent;
                while (node != nil && node->right != nil) node = node->right;
            } 
            else if (node->left != nil) 
            {
                node = node->left;
                while (node->right != nil) 
                {
                    node = node->right;
                }
            } 
            else 
            {
                BasePtr y = node->parent;
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

        bool operator==(const RBTreeIterator& other) const noexcept { 
            return node == other.node && nil == other.nil; 
        }
        
        bool operator!=(const RBTreeIterator& other) const noexcept { 
            return !(*this == other); 
        }
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
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

    private:
        using Node = RBNode<Value>;
        using BasePtr = RBNodeBase*;
        
        using node_allocator_type = typename mystl::allocator_traits<Allocator>::template rebind_alloc<Node>;
        using node_traits = mystl::allocator_traits<node_allocator_type>;

        // Allocator specifically for RBNodeBase to create nil_ without calling Value constructor (fix for issue #1)
        using base_allocator_type = typename mystl::allocator_traits<Allocator>::template rebind_alloc<RBNodeBase>;
        using base_traits = mystl::allocator_traits<base_allocator_type>;

        [[no_unique_address]] node_allocator_type alloc_;
        [[no_unique_address]] base_allocator_type base_alloc_;
        [[no_unique_address]] Compare             comp_;
        [[no_unique_address]] KeyOfValue          extract_key_;

        BasePtr   root_;
        BasePtr   nil_;
        size_type size_;

        Node* allocate_node() { return node_traits::allocate(alloc_, 1); }
        void deallocate_node(Node* p) { node_traits::deallocate(alloc_, p, 1); }

        template <typename... Args>
        Node* create_node(BasePtr parent, BasePtr left, BasePtr right, RBColor color, Args&&... args) 
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
            nil_ = base_traits::allocate(base_alloc_, 1);
            base_traits::construct(base_alloc_, nil_, nil_, nil_, nil_, RBColor::Black);
            root_ = nil_;
        }

    public:
        RBTree(const Compare& comp = Compare(), const Allocator& alloc = Allocator())
            : alloc_(alloc)
            , base_alloc_(alloc)
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
            if (nil_) 
            {
                clear();
                base_traits::destroy(base_alloc_, nil_);
                base_traits::deallocate(base_alloc_, nil_, 1);
            }
        }

        RBTree(const RBTree& other)
            : alloc_(mystl::allocator_traits<allocator_type>::select_on_container_copy_construction(other.alloc_))
            , base_alloc_(mystl::allocator_traits<base_allocator_type>::select_on_container_copy_construction(other.base_alloc_))
            , comp_(other.comp_), extract_key_(other.extract_key_), root_(nullptr), nil_(nullptr), size_(0)
        {
            init_nil();
            if (other.nil_ && other.root_ != other.nil_)
            {
                root_ = copy_tree(other.root_, nil_, other.nil_);
                size_ = other.size_;
            }
        }

        RBTree(const RBTree& other, const Allocator& alloc)
            : alloc_(alloc)
            , base_alloc_(alloc)
            , comp_(other.comp_)
            , extract_key_(other.extract_key_)
            , root_(nullptr)
            , nil_(nullptr)
            , size_(0)
        {
            init_nil();
            if (other.nil_ && other.root_ != other.nil_)
            {
                root_ = copy_tree(other.root_, nil_, other.nil_);
                size_ = other.size_;
            }
        }

        RBTree(RBTree&& other) noexcept
            : alloc_(mystl::move(other.alloc_))
            , base_alloc_(mystl::move(other.base_alloc_))
            , comp_(mystl::move(other.comp_))
            , extract_key_(mystl::move(other.extract_key_))
            , root_(other.root_)
            , nil_(other.nil_)
            , size_(other.size_)
        {
            other.root_ = nullptr;
            other.nil_ = nullptr;
            other.size_ = 0;
        }

        RBTree(RBTree&& other, const Allocator& alloc) noexcept
            : alloc_(alloc)
            , base_alloc_(alloc)
            , comp_(mystl::move(other.comp_))
            , extract_key_(mystl::move(other.extract_key_))
            , root_(other.root_)
            , nil_(other.nil_)
            , size_(other.size_)
        {
            other.root_ = nullptr;
            other.nil_ = nullptr;
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

            if (nil_) 
            {
                clear();
                base_traits::destroy(base_alloc_, nil_);
                base_traits::deallocate(base_alloc_, nil_, 1);
            }

            alloc_ = mystl::move(other.alloc_);
            base_alloc_ = mystl::move(other.base_alloc_);
            comp_ = mystl::move(other.comp_);
            extract_key_ = mystl::move(other.extract_key_);

            root_ = other.root_;
            nil_ = other.nil_;
            size_ = other.size_;

            other.root_ = nullptr;
            other.nil_ = nullptr;
            other.size_ = 0;

            return *this;
        }

        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

        // Added const versions of begin/end
        iterator begin() noexcept { return nil_ ? iterator(RBTreeAlgorithms::minimum(root_, nil_), nil_) : iterator(nullptr, nullptr); }
        iterator end() noexcept { return iterator(nil_, nil_); }
        
        const_iterator begin() const noexcept { return nil_ ? const_iterator(RBTreeAlgorithms::minimum(root_, nil_), nil_) : const_iterator(nullptr, nullptr); }
        const_iterator end() const noexcept { return const_iterator(nil_, nil_); }
        
        const_iterator cbegin() const noexcept { return begin(); }
        const_iterator cend() const noexcept { return end(); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
        const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        template <typename... Args>
        mystl::Pair<iterator, bool> emplace_unique(Args&&... args) 
        {
            // Lazy initialization for moved-from containers
            if (!nil_) 
                init_nil();

            Node* z = create_node(nullptr, nil_, nil_, RBColor::Red, mystl::forward<Args>(args)...);
            const Key& key = extract_key_(z->value);

            BasePtr y = nil_;
            BasePtr x = root_;

            while (x != nil_) 
            {
                y = x;
                const Key& x_key = extract_key_(static_cast<Node*>(x)->value);
                
                if (comp_(key, x_key)) 
                    x = x->left;
                else if (comp_(x_key, key)) 
                    x = x->right;
                else 
                {
                    destroy_node(z);
                    return { iterator(x, nil_), false };
                }
            }

            z->parent = y;
            if (y == nil_) root_ = z;
            else if (comp_(key, extract_key_(static_cast<Node*>(y)->value))) y->left = z;
            else y->right = z;

            nil_->parent = root_;
            RBTreeAlgorithms::insert_fixup(z, root_, nil_);
            ++size_;
            return { iterator(z, nil_), true };
        }

        template <typename... Args>
        iterator emplace_equal(Args&&... args)
        {
            if (!nil_) 
                init_nil();

            Node* z = create_node(nullptr, nil_, nil_, RBColor::Red, mystl::forward<Args>(args)...);
            const Key& key = extract_key_(z->value);

            BasePtr y = nil_;
            BasePtr x = root_;

            while (x != nil_)
            {
                y = x;
                if (comp_(key, extract_key_(static_cast<Node*>(x)->value)))
                    x = x->left;
                else
                    x = x->right;
            }

            z->parent = y;
            if (y == nil_) root_ = z;
            else if (comp_(key, extract_key_(static_cast<Node*>(y)->value))) y->left = z;
            else y->right = z;

            nil_->parent = root_;
            RBTreeAlgorithms::insert_fixup(z, root_, nil_);
            ++size_;
            return iterator(z, nil_);
        }

        void clear() noexcept
        {
            if (!nil_) 
                return; // Guard for moved-from trees
            clear_helper(root_);
            root_ = nil_;
            size_ = 0;
            nil_->parent = nil_;
        }

        iterator find(const Key& key) noexcept 
        {
            if (!nil_) return end();
            BasePtr current = root_;
            while (current != nil_) 
            {
                const Key& curr_key = extract_key_(static_cast<Node*>(current)->value);
                if (comp_(key, curr_key)) current = current->left;
                else if (comp_(curr_key, key)) current = current->right;
                else return iterator(current, nil_);
            }
            return end();
        }

        const_iterator find(const Key& key) const noexcept 
        {
            if (!nil_) return end();
            BasePtr current = root_;
            while (current != nil_) 
            {
                const Key& curr_key = extract_key_(static_cast<Node*>(current)->value);
                if (comp_(key, curr_key)) current = current->left;
                else if (comp_(curr_key, key)) current = current->right;
                else return const_iterator(current, nil_);
            }
            return cend();
        }

        bool contains(const Key& key) const noexcept 
        {
            return find(key) != end();
        }

        iterator lower_bound(const Key& key) noexcept
        {
            if (!nil_) return end();
            BasePtr current = root_;
            BasePtr result = nil_;
            while (current != nil_)
            {
                if (!comp_(extract_key_(static_cast<Node*>(current)->value), key))
                {
                    result = current;
                    current = current->left;
                }
                else current = current->right;
            }
            return iterator(result, nil_);
        }

        const_iterator lower_bound(const Key& key) const noexcept
        {
            if (!nil_) return end();
            BasePtr current = root_;
            BasePtr result = nil_;
            while (current != nil_)
            {
                if (!comp_(extract_key_(static_cast<Node*>(current)->value), key))
                {
                    result = current;
                    current = current->left;
                }
                else current = current->right;
            }
            return const_iterator(result, nil_);
        }

        iterator upper_bound(const Key& key) noexcept 
        {
            if (!nil_) return end();
            BasePtr current = root_;
            BasePtr result = nil_;
            while (current != nil_) 
            {
                if (comp_(key, extract_key_(static_cast<Node*>(current)->value))) 
                {
                    result = current; 
                    current = current->left;
                } 
                else current = current->right;
            }
            return iterator(result, nil_);
        }

        const_iterator upper_bound(const Key& key) const noexcept 
        {
            if (!nil_) return end();
            BasePtr current = root_;
            BasePtr result = nil_;
            while (current != nil_) 
            {
                if (comp_(key, extract_key_(static_cast<Node*>(current)->value))) 
                {
                    result = current; 
                    current = current->left;
                } 
                else current = current->right;
            }
            return const_iterator(result, nil_);
        }

        size_type erase(const Key& key)
        {
            iterator it = find(key);
            if (it == end()) 
                return 0;

            delete_node(it.node);
            return 1;
        }

        iterator erase(iterator pos)
        {
            iterator next = pos;
            ++next;
            delete_node(pos.node);
            return next;
        }

        iterator erase(const_iterator pos)
        {
            iterator next = iterator(pos.node, pos.nil);
            ++next;
            delete_node(pos.node);
            return next;
        }

        void swap(RBTree& other) noexcept 
        {
            mystl::swap(alloc_, other.alloc_);
            mystl::swap(base_alloc_, other.base_alloc_);
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

        void clear_helper(BasePtr node) noexcept 
        {
            if (node == nil_) return;
            clear_helper(node->left);
            clear_helper(node->right);
            destroy_node(static_cast<Node*>(node));
        }

        BasePtr copy_tree(BasePtr source_node, BasePtr parent_node, BasePtr source_nil) 
        {
            if (source_node == source_nil) return nil_;
            
            Node* sn = static_cast<Node*>(source_node);
            Node* new_node = create_node(parent_node, nil_, nil_, sn->color, sn->value);
            
            try 
            {
                new_node->left = copy_tree(sn->left, new_node, source_nil);
                new_node->right = copy_tree(sn->right, new_node, source_nil);
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

        void delete_node(BasePtr z) 
        {
            BasePtr x;
            BasePtr y = z;
            RBColor y_original_color = y->color;

            if (z->left == nil_) 
            {
                x = z->right;
                RBTreeAlgorithms::transplant(z, z->right, root_, nil_);
            } 
            else if (z->right == nil_) 
            {
                x = z->left;
                RBTreeAlgorithms::transplant(z, z->left, root_, nil_);
            } 
            else 
            {
                y = RBTreeAlgorithms::minimum(z->right, nil_);
                y_original_color = y->color;
                x = y->right;

                if (y->parent == z) x->parent = y;
                else 
                {
                    RBTreeAlgorithms::transplant(y, y->right, root_, nil_);
                    y->right = z->right;
                    y->right->parent = y;
                }

                RBTreeAlgorithms::transplant(z, y, root_, nil_);
                y->left = z->left;
                y->left->parent = y;
                y->color = z->color;
            }

            destroy_node(static_cast<Node*>(z)); 
            --size_;

            if (y_original_color == RBColor::Black) 
            {
                RBTreeAlgorithms::erase_fixup(x, root_, nil_);
            }
            
            if (root_ != nil_) root_->parent = nil_;
            nil_->parent = root_; 
        }
    };

} // namespace mystl