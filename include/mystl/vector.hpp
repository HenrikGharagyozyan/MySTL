#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "memory.hpp"
#include "allocator.hpp"
#include "algorithm.hpp"

#include <cstddef>
#include <initializer_list>
#include <stdexcept>

namespace mystl 
{
    template <typename T, typename Allocator = mystl::Allocator<T>>
    class Vector 
    {
    public:
        // ========================================================================
        // TYPES & ALIASES
        // ========================================================================
        using value_type             = T;
        using allocator_type         = Allocator;
        using allocator_traits_type  = mystl::allocator_traits<Allocator>;
        using size_type              = typename allocator_traits_type::size_type;
        using difference_type        = std::ptrdiff_t;
        using reference              = T&;
        using const_reference        = const T&;
        using pointer                = typename allocator_traits_type::pointer;
        using const_pointer          = const T*;
        
        using iterator               = T*;
        using const_iterator         = const T*;
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

    private:
        pointer   data_      = nullptr;
        size_type size_      = 0;
        size_type capacity_  = 0;
        
        [[no_unique_address]] Allocator alloc_;

    public:
        // ========================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // ========================================================================
        Vector() noexcept(noexcept(Allocator())) = default;

        explicit Vector(const Allocator& alloc) noexcept : alloc_(alloc) {}

        explicit Vector(size_type count, const T& value = T(), const Allocator& alloc = Allocator())
            : size_(count), capacity_(count), alloc_(alloc) 
        {
            if (count > 0)
            {
                data_ = allocator_traits_type::allocate(alloc_, capacity_);
                try
                {
                    mystl::uninitialized_fill(data_, data_ + size_, value);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        Vector(std::initializer_list<T> init, const Allocator& alloc = Allocator())
            : size_(init.size()), capacity_(init.size()), alloc_(alloc) 
        {
            if (size_ > 0)
            {
                data_ = allocator_traits_type::allocate(alloc_, capacity_);
                try
                {
                    mystl::uninitialized_copy(init.begin(), init.end(), data_);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        template <typename InputIt, typename = mystl::enable_if_t<!mystl::is_integral<InputIt>::value>>
        Vector(InputIt first, InputIt last, const Allocator& alloc = Allocator())
            : alloc_(alloc) 
        {
            size_type count = mystl::distance(first, last);
            if (count > 0) 
            {
                capacity_ = size_ = count;
                data_ = allocator_traits_type::allocate(alloc_, capacity_);
                try
                {
                    mystl::uninitialized_copy(first, last, data_);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        // ========================================================================
        // RULE OF FIVE
        // ========================================================================
        Vector(const Vector& other)
            : size_(other.size_), capacity_(other.size_)
            , alloc_(allocator_traits_type::select_on_container_copy_construction(other.alloc_))
        {
            if (capacity_ > 0)
            {
                data_ = allocator_traits_type::allocate(alloc_, capacity_);
                try
                {
                    mystl::uninitialized_copy(other.data_, other.data_ + other.size_, data_);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        Vector(const Vector& other, const Allocator& alloc)
            : alloc_(alloc)
        {
            if (other.size_ > 0)
            {
                capacity_ = size_ = other.size_;
                data_ = allocator_traits_type::allocate(alloc_, capacity_);
                try
                {
                    mystl::uninitialized_copy(other.begin(), other.end(), data_);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    throw;
                }
            }
        }

        Vector(Vector&& other) noexcept
            : data_(other.data_), size_(other.size_), capacity_(other.capacity_), alloc_(mystl::move(other.alloc_)) 
        {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        Vector(Vector&& other, const Allocator& alloc)
            : alloc_(alloc)
        {
            if (alloc_ == other.alloc_)
            {
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            else
            {
                reserve(other.size_);

                for (auto& x : other)
                    push_back(mystl::move(x));
            }
        }

        Vector& operator=(const Vector& other)
        {
            if (this != &other)
            {
                // Strong guarantee: build the full copy in fresh storage first and
                // commit only once it has succeeded. *this is left untouched if the
                // allocation or any element copy throws, and size_ never describes
                // raw memory.
                pointer   new_data = nullptr;
                size_type new_cap  = other.size_;

                if (new_cap > 0)
                {
                    new_data = allocator_traits_type::allocate(alloc_, new_cap);
                    try
                    {
                        mystl::uninitialized_copy(other.data_, other.data_ + other.size_, new_data);
                    }
                    catch (...)
                    {
                        allocator_traits_type::deallocate(alloc_, new_data, new_cap);
                        throw;
                    }
                }

                // Commit: nothing below can throw.
                mystl::destroy(data_, data_ + size_);
                if (data_) allocator_traits_type::deallocate(alloc_, data_, capacity_);

                data_     = new_data;
                size_     = other.size_;
                capacity_ = new_cap;
            }
            return *this;
        }

        Vector& operator=(Vector&& other) noexcept 
        {
            if (this != &other) 
            {
                clear();
                if (data_) allocator_traits_type::deallocate(alloc_, data_, capacity_);
                
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;
                alloc_    = mystl::move(other.alloc_);

                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        ~Vector() 
        {
            if (data_) 
            {
                mystl::destroy(data_, data_ + size_);
                allocator_traits_type::deallocate(alloc_, data_, capacity_);
            }
        }

        // ========================================================================
        // ELEMENT ACCESS
        // ========================================================================
        reference operator[](size_type index) { return data_[index]; }
        const_reference operator[](size_type index) const { return data_[index]; }

        reference at(size_type index) 
        {
            if (index >= size_) throw std::out_of_range("Vector::at");
            return data_[index];
        }
        
        const_reference at(size_type index) const 
        {
            if (index >= size_) throw std::out_of_range("Vector::at");
            return data_[index];
        }

        reference front() { return data_[0]; }
        const_reference front() const { return data_[0]; }

        reference back() { return data_[size_ - 1]; }
        const_reference back() const { return data_[size_ - 1]; }

        pointer data() noexcept { return data_; }
        const_pointer data() const noexcept { return data_; }

        // ========================================================================
        // ITERATORS    
        // ========================================================================
        iterator begin() noexcept { return data_; }
        iterator end() noexcept { return data_ + size_; }
        const_iterator begin() const noexcept { return data_; }
        const_iterator end() const noexcept { return data_ + size_; }
        const_iterator cbegin() const noexcept { return data_; }
        const_iterator cend() const noexcept { return data_ + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ========================================================================
        // CAPACITY
        // ========================================================================
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
        
        [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

        void reserve(size_type new_cap)
        {
            if (new_cap > capacity_)
            {
                pointer new_data = allocator_traits_type::allocate(alloc_, new_cap);

                // Strong guarantee: move only when T's move cannot throw, otherwise
                // copy, so the source range survives if a transfer throws. If it does
                // throw, release new_data before propagating to avoid a leak.
                try
                {
                    mystl::uninitialized_move_if_noexcept(data_, data_ + size_, new_data);
                }
                catch (...)
                {
                    allocator_traits_type::deallocate(alloc_, new_data, new_cap);
                    throw;
                }

                mystl::destroy(data_, data_ + size_);

                if (data_) allocator_traits_type::deallocate(alloc_, data_, capacity_);

                data_ = new_data;
                capacity_ = new_cap;
            }
        }

        void shrink_to_fit() 
        {
            if (size_ < capacity_) 
            {
                if (size_ == 0) 
                {
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    data_ = nullptr;
                    capacity_ = 0;
                } 
                else
                {
                    pointer new_data = allocator_traits_type::allocate(alloc_, size_);
                    try
                    {
                        mystl::uninitialized_move_if_noexcept(data_, data_ + size_, new_data);
                    }
                    catch (...)
                    {
                        allocator_traits_type::deallocate(alloc_, new_data, size_);
                        throw;
                    }
                    mystl::destroy(data_, data_ + size_);
                    allocator_traits_type::deallocate(alloc_, data_, capacity_);
                    data_ = new_data;
                    capacity_ = size_;
                }
            }
        }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void clear() noexcept 
        {
            mystl::destroy(data_, data_ + size_);
            size_ = 0;
        }

        void push_back(const T& value) 
        {
            if (size_ == capacity_) reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            allocator_traits_type::construct(alloc_, data_ + size_, value);
            ++size_;
        }

        void push_back(T&& value) 
        {
            if (size_ == capacity_) reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            allocator_traits_type::construct(alloc_, data_ + size_, mystl::move(value));
            ++size_;
        }

        template <typename... Args>
        reference emplace_back(Args&&... args) 
        {
            if (size_ == capacity_) reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            allocator_traits_type::construct(alloc_, data_ + size_, mystl::forward<Args>(args)...);
            return data_[size_++];
        }

        void pop_back() 
        {
            if (size_ > 0) 
            {
                --size_;
                mystl::destroy_at(data_ + size_);
            }
        }

        iterator insert(const_iterator pos, const T& value) 
        {
            size_type index = pos - data_;
            if (size_ == capacity_) reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            
            pointer insert_pos = data_ + index;
            if (index == size_) 
            {
                allocator_traits_type::construct(alloc_, insert_pos, value);
            } 
            else 
            {
                // Shift elements to the right
                allocator_traits_type::construct(alloc_, data_ + size_, mystl::move(data_[size_ - 1]));
                for (size_type i = size_ - 1; i > index; --i) 
                {
                    data_[i] = mystl::move(data_[i - 1]);
                }
                data_[index] = value;
            }
            ++size_;
            return insert_pos;
        }

        iterator erase(const_iterator pos) 
        {
            size_type index = pos - data_;
            pointer erase_pos = data_ + index;
            
            for (size_type i = index; i < size_ - 1; ++i) 
            {
                data_[i] = mystl::move(data_[i + 1]);
            }
            --size_;
            mystl::destroy_at(data_ + size_);
            return erase_pos;
        }

        void swap(Vector& other) noexcept 
        {
            mystl::swap(data_, other.data_);
            mystl::swap(size_, other.size_);
            mystl::swap(capacity_, other.capacity_);
            mystl::swap(alloc_, other.alloc_);
        }
    };

    // ========================================================================
    // NON-MEMBER FUNCTIONS
    // ========================================================================
    template <typename T, typename Alloc>
    bool operator==(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return lhs.size() == rhs.size() && mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template <typename T, typename Alloc>
    bool operator!=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return !(lhs == rhs);
    }

    template <typename T, typename Alloc>
    bool operator<(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return mystl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename Alloc>
    bool operator>(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return rhs < lhs;
    }

    template <typename T, typename Alloc>
    bool operator<=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return !(rhs < lhs);
    }

    template <typename T, typename Alloc>
    bool operator>=(const Vector<T, Alloc>& lhs, const Vector<T, Alloc>& rhs) 
    {
        return !(lhs < rhs);
    }

    template <typename T, typename Alloc>
    void swap(Vector<T, Alloc>& lhs, Vector<T, Alloc>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl