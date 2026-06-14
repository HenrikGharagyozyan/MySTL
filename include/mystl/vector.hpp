#pragma once

#include <cstddef>
#include <stdexcept>
#include <initializer_list>
#include <iterator>
#include "allocator.hpp"
#include "utility.hpp"

namespace mystl
{

    template <typename T, typename Allocator = mystl::Allocator<T>>
    class Vector
    {
    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using reference = T& ;
        using const_reference = const T& ;
        using pointer = T *;
        using const_pointer = const T *;

        // Forward declaration of ConstIterator for conversion operator.
        class ConstIterator;

        class Iterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Vector::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = Vector::pointer;
            using reference = Vector::reference;

        private:
            pointer ptr_;

        public:
            Iterator(pointer ptr = nullptr) : ptr_(ptr) {}

            reference operator*() const { return *ptr_; }
            pointer operator->() const { return ptr_; }

            Iterator& operator++()
            {
                ++ptr_;
                return *this;
            }
            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++ptr_;
                return temp;
            }
            Iterator& operator--()
            {
                --ptr_;
                return *this;
            }
            Iterator operator--(int)
            {
                Iterator temp = *this;
                --ptr_;
                return temp;
            }

            Iterator operator+(difference_type dif) const { return Iterator(ptr_ + dif); }
            Iterator operator-(difference_type dif) const { return Iterator(ptr_ - dif); }
            difference_type operator-(const Iterator& rhs) const { return ptr_ - rhs.ptr_; }

            bool operator==(const Iterator& rhs) const { return ptr_ == rhs.ptr_; }
            bool operator!=(const Iterator& rhs) const { return ptr_ != rhs.ptr_; }
            bool operator<(const Iterator& rhs) const { return ptr_ < rhs.ptr_; }
            bool operator>(const Iterator& rhs) const { return ptr_ > rhs.ptr_; }
            bool operator<=(const Iterator& rhs) const { return ptr_ <= rhs.ptr_; }
            bool operator>=(const Iterator& rhs) const { return ptr_ >= rhs.ptr_; }

            Iterator& operator+=(difference_type dif)
            {
                ptr_ += dif;
                return *this;
            }
            Iterator& operator-=(difference_type dif)
            {
                ptr_ -= dif;
                return *this;
            }
            reference operator[](difference_type dif) const { return *(ptr_ + dif); }

            // Access to the raw pointer for container needs.
            pointer base() const { return ptr_; }
        };

        class ConstIterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = Vector::value_type;
            using difference_type = std::ptrdiff_t;
            using pointer = Vector::const_pointer;
            using reference = Vector::const_reference;

        private:
            const_pointer ptr_;

        public:
            ConstIterator(const_pointer ptr = nullptr) : ptr_(ptr) {}
            // Conversion constructor from non-const iterator to const iterator.
            ConstIterator(const Iterator& it) : ptr_(it.base()) {}

            reference operator*() const { return *ptr_; }
            pointer operator->() const { return ptr_; }

            ConstIterator& operator++()
            {
                ++ptr_;
                return *this;
            }
            ConstIterator operator++(int)
            {
                ConstIterator temp = *this;
                ++ptr_;
                return temp;
            }
            ConstIterator& operator--()
            {
                --ptr_;
                return *this;
            }
            ConstIterator operator--(int)
            {
                ConstIterator temp = *this;
                --ptr_;
                return temp;
            }

            ConstIterator operator+(difference_type dif) const { return ConstIterator(ptr_ + dif); }
            ConstIterator operator-(difference_type dif) const { return ConstIterator(ptr_ - dif); }
            difference_type operator-(const ConstIterator& rhs) const { return ptr_ - rhs.ptr_; }

            bool operator==(const ConstIterator& rhs) const { return ptr_ == rhs.ptr_; }
            bool operator!=(const ConstIterator& rhs) const { return ptr_ != rhs.ptr_; }
            bool operator<(const ConstIterator& rhs) const { return ptr_ < rhs.ptr_; }
            bool operator>(const ConstIterator& rhs) const { return ptr_ > rhs.ptr_; }
            bool operator<=(const ConstIterator& rhs) const { return ptr_ <= rhs.ptr_; }
            bool operator>=(const ConstIterator& rhs) const { return ptr_ >= rhs.ptr_; }

            ConstIterator& operator+=(difference_type dif)
            {
                ptr_ += dif;
                return *this;
            }
            ConstIterator& operator-=(difference_type dif)
            {
                ptr_ -= dif;
                return *this;
            }
            reference operator[](difference_type dif) const { return *(ptr_ + dif); }

            const_pointer base() const { return ptr_; }

            friend bool operator==(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ == rhs.ptr_;
            }
            friend bool operator!=(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ != rhs.ptr_;
            }
            friend bool operator<(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ < rhs.ptr_;
            }
            friend bool operator>(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ > rhs.ptr_;
            }
            friend bool operator<=(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ <= rhs.ptr_;
            }
            friend bool operator>=(const ConstIterator& lhs, const ConstIterator& rhs) noexcept
            {
                return lhs.ptr_ >= rhs.ptr_;
            }
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    private:
        pointer data_ = nullptr;
        size_type size_ = 0;
        size_type capacity_ = 0;
        allocator_type alloc_;

        // Internal reallocation function with strong exception guarantee.
        void reallocate(size_type new_capacity)
        {
            pointer new_data = alloc_.allocate(new_capacity);
            size_type i = 0;
            try
            {
                // Move existing elements to the new buffer.
                for (; i < size_; ++i)
                {
                    // Safety: move only if it doesn't throw exceptions.
                    alloc_.construct(new_data + i, mystl::move(data_[i]));
                }
            }
            catch (...)
            {
                // Rollback in case of an exception.
                for (size_type j = 0; j < i; ++j)
                {
                    alloc_.destroy(new_data + j);
                }
                alloc_.deallocate(new_data, new_capacity);
                throw;
            }

            // Clean up the old buffer.
            for (size_type j = 0; j < size_; ++j)
            {
                alloc_.destroy(data_ + j);
            }
            if (data_)
            {
                alloc_.deallocate(data_, capacity_);
            }

            data_ = new_data;
            capacity_ = new_capacity;
        }

    public:
        // ========================================================================
        // CONSTRUCTORS AND RULE OF FIVE
        // ========================================================================
        Vector() noexcept = default;

        explicit Vector(size_type count)
            : size_(count)
            , capacity_(count * 2)
        {
            data_ = capacity_ > 0 ? alloc_.allocate(capacity_) : nullptr;
            for (size_type i = 0; i < size_; ++i)
                alloc_.construct(data_ + i);
        }

        Vector(size_type count, const_reference value)
            : size_(count)
            , capacity_(count * 2)
        {
            data_ = capacity_ > 0 ? alloc_.allocate(capacity_) : nullptr;
            for (size_type i = 0; i < size_; ++i)
                alloc_.construct(data_ + i, value);
        }

        Vector(std::initializer_list<T> init)
            : size_(init.size())
            , capacity_(init.size() * 2)
        {
            data_ = capacity_ > 0 ? alloc_.allocate(capacity_) : nullptr;
            size_type i = 0;
            for (const auto& value : init)
                alloc_.construct(data_ + i++, value);
        }

        ~Vector()
        {
            clear();
            if (data_)
                alloc_.deallocate(data_, capacity_);
        }

        Vector(const Vector& other)
            : size_(other.size_)
            , capacity_(other.capacity_)
        {
            data_ = capacity_ > 0 ? alloc_.allocate(capacity_) : nullptr;
            for (size_type i = 0; i < size_; ++i)
                alloc_.construct(data_ + i, other.data_[i]);
        }

        Vector(Vector&& other) noexcept
            : data_(other.data_)
            , size_(other.size_)
            , capacity_(other.capacity_)
            , alloc_(mystl::move(other.alloc_))
        {
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }

        Vector& operator=(const Vector& other)
        {
            if (this != &other)
            {
                Vector temp(other);
                swap(temp);
            }
            return *this;
        }

        Vector& operator=(Vector& other) noexcept
        {
            if (this != &other)
            {
                clear();
                if (data_)
                    alloc_.deallocate(data_, capacity_);
                data_ = other.data_;
                size_ = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = 0;
                other.capacity_ = 0;
            }
            return *this;
        }

        // ========================================================================
        // ITERATOR CLASSES
        // ========================================================================
        iterator begin() noexcept { return iterator(data_); }
        iterator end() noexcept { return iterator(data_ + size_); }
        const_iterator begin() const noexcept { return const_iterator(data_); }
        const_iterator end() const noexcept { return const_iterator(data_ + size_); }
        const_iterator cbegin() const noexcept { return const_iterator(data_); }
        const_iterator cend() const noexcept { return const_iterator(data_ + size_); }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

        // ========================================================================
        // CAPACITY AND ACCESS INTERFACE
        // ========================================================================
        size_type size() const noexcept { return size_; }
        size_type capacity() const noexcept { return capacity_; }
        bool empty() const noexcept { return size_ == 0; }

        reference operator[](size_type index) { return data_[index]; }
        const_reference operator[](size_type index) const { return data_[index]; }

        reference at(size_type index)
        {
            if (index >= size_)
                throw std::out_of_range("Vector::at out of range");
            return data_[index];
        }
        const_reference at(size_type index) const
        {
            if (index >= size_)
                throw std::out_of_range("Vector::at out of range");
            return data_[index];
        }

        reference front() { return data_[0]; }
        const_reference front() const { return data_[0]; }
        reference back() { return data_[size_ - 1]; }
        const_reference back() const { return data_[size_ - 1]; }

        // ========================================================================
        // MODIFIERS
        // ========================================================================
        void reserve(size_type new_capacity)
        {
            if (new_capacity > capacity_)
                reallocate(new_capacity);
        }

        void clear() noexcept
        {
            for (size_type i = 0; i < size_; ++i)
                alloc_.destroy(data_ + i);
            size_ = 0;
        }

        void push_back(const_reference value)
        {
            if (size_ == capacity_)
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            alloc_.construct(data_ + size_, value);
            ++size_;
        }

        void push_back(value_type& value)
        {
            if (size_ == capacity_)
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            alloc_.construct(data_ + size_, mystl::move(value));
            ++size_;
        }

        template <typename... Args>
        void emplace_back(Args& ...args)
        {
            if (size_ == capacity_)
                reserve(capacity_ == 0 ? 1 : capacity_ * 2);
            alloc_.construct(data_ + size_, mystl::forward<Args>(args)...);
            ++size_;
        }

        void pop_back()
        {
            if (size_ > 0)
            {
                --size_;
                alloc_.destroy(data_ + size_);
            }
        }

        void swap(Vector& other) noexcept
        {
            mystl::swap(data_, other.data_);
            mystl::swap(size_, other.size_);
            mystl::swap(capacity_, other.capacity_);
        }

        // ========================================================================
        // MODIFIERS (INSERT AND ERASE)
        // ========================================================================

        // Insert an element at a specific iterator position.
        iterator insert(const_iterator pos, const T& value)
        {
            // Calculate the insertion index using the difference of base pointers.
            size_type index = pos.base() - data_;

            if (size_ == capacity_)
            {
                // If there's no space, perform reallocation combined with insertion.
                size_type new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
                pointer new_data = alloc_.allocate(new_capacity);

                size_type i = 0;
                try
                {
                    // 1. Copy/move elements BEFORE the insertion index.
                    for (; i < index; ++i)
                        alloc_.construct(new_data + i, mystl::move(data_[i]));

                    // 2. Construct the new element at its position.
                    alloc_.construct(new_data + index, value);
                    // 3. Copy/move elements AFTER the insertion index.
                    for (i = index; i < size_; ++i)
                        alloc_.construct(new_data + i + 1, mystl::move(data_[i]));
                }
                catch (...)
                {
                    // Emergency rollback in case of an exception.
                    for (size_type j = 0; j < i + (i > index ? 1 : 0); ++j)
                        alloc_.destroy(new_data + j);

                    alloc_.deallocate(new_data, new_capacity);
                    throw;
                }

                // Clear the old buffer.
                for (size_type j = 0; j < size_; ++j)
                    alloc_.destroy(data_ + j);

                if (data_)
                    alloc_.deallocate(data_, capacity_);

                data_ = new_data;
                capacity_ = new_capacity;
            }
            else
            {
                // If there is space, shift elements to the right in the current buffer.
                if (index < size_)
                {
                    // Construct a copy of the last element at the new (uninitialized) position.
                    alloc_.construct(data_ + size_, mystl::move(data_[size_ - 1]));

                    // Shift remaining elements using assignment operator from right to left.
                    for (size_type i = size_ - 1; i > index; --i)
                        data_[i] = mystl::move(data_[i - 1]);

                    // Assign the new value to the freed position.
                    data_[index] = value;
                }
                else
                {
                    // If inserting at the very end (essentially push_back).
                    alloc_.construct(data_ + index, value);
                }
            }

            ++size_;
            return iterator(data_ + index);
        }

        // Insert an rvalue element (move).
        iterator insert(const_iterator pos, T&& value)
        {
            size_type index = pos.base() - data_;

            if (size_ == capacity_)
            {
                size_type new_capacity = (capacity_ == 0 ? 1 : capacity_ * 2);
                pointer new_data = alloc_.allocate(new_capacity);

                size_type i = 0;
                try
                {
                    for (; i < index; ++i)
                        alloc_.construct(new_data + i, mystl::move(data_[i]));

                    // Move the value itself.
                    alloc_.construct(new_data + index, mystl::move(value));

                    for (i = index; i < size_; ++i)
                        alloc_.construct(new_data + i + 1, mystl::move(data_[i]));
                }
                catch (...)
                {
                    for (size_type j = 0; j < i + (i > index ? 1 : 0); ++j)
                        alloc_.destroy(new_data + j);
                    alloc_.deallocate(new_data, new_capacity);
                    throw;
                }

                for (size_type j = 0; j < size_; ++j)
                    alloc_.destroy(data_ + j);
                if (data_)
                    alloc_.deallocate(data_, capacity_);

                data_ = new_data;
                capacity_ = new_capacity;
            }
            else
            {
                if (index < size_)
                {
                    alloc_.construct(data_ + size_, mystl::move(data_[size_ - 1]));
                    for (size_type i = size_ - 1; i > index; --i)
                        data_[i] = mystl::move(data_[i - 1]);

                    data_[index] = mystl::move(value); // Move assignment.
                }
                else
                {
                    alloc_.construct(data_ + index, mystl::move(value));
                }
            }

            ++size_;
            return iterator(data_ + index);
        }

        // Erase an element at a specific iterator position.
        iterator erase(const_iterator pos)
        {
            size_type index = pos.base() - data_;

            // Shift all elements from left to right, overwriting the element to be erased.
            for (size_type i = index; i < size_ - 1; ++i)
                data_[i] = mystl::move(data_[i + 1]);

            // Destroy the now-unneeded last element
            --size_;
            alloc_.destroy(data_ + size_);

            return iterator(data_ + index);
        }

        pointer data() noexcept { return data_; }
        const_pointer data() const noexcept { return data_; }

    };

    template <typename T>
    void swap(Vector<T>& lhs, Vector<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

} // namespace mystl