#pragma once

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include "allocator.hpp"
#include "utility.hpp"

namespace mystl
{

    class String
    {
    public:
        using value_type = char;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference = value_type& ;
        using const_reference = const value_type& ;
        using pointer = value_type* ;
        using const_pointer = const value_type* ;

        // ========================================================================
        // NESTED ITERATOR CLASSES
        // ========================================================================
        class ConstIterator;

        class Iterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = char;
            using difference_type = std::ptrdiff_t;
            using pointer = char* ;
            using reference = char& ;

        private:
            pointer ptr_ = nullptr;
            friend class String;
            friend class ConstIterator;
            explicit Iterator(pointer ptr) : ptr_(ptr) {}

        public:
            Iterator() = default;

            reference operator*() const { return* ptr_; }
            pointer operator->() const { return ptr_; }

            Iterator& operator++()
            {
                ++ptr_;
                return* this;
            }
            Iterator operator++(int)
            {
                Iterator tmp =* this;
                ++ptr_;
                return tmp;
            }
            Iterator& operator--()
            {
                --ptr_;
                return* this;
            }
            Iterator operator--(int)
            {
                Iterator tmp =* this;
                --ptr_;
                return tmp;
            }

            Iterator& operator+=(difference_type n)
            {
                ptr_ += n;
                return* this;
            }
            Iterator& operator-=(difference_type n)
            {
                ptr_ -= n;
                return* this;
            }

            Iterator operator+(difference_type n) const { return Iterator(ptr_ + n); }
            Iterator operator-(difference_type n) const { return Iterator(ptr_ - n); }
            difference_type operator-(const Iterator& other) const { return ptr_ - other.ptr_; }

            reference operator[](difference_type n) const { return ptr_[n]; }

            friend bool operator==(const Iterator& lhs, const Iterator& rhs) noexcept
            {
                return lhs.ptr_ == rhs.ptr_;
            }
        };

        class ConstIterator
        {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = char;
            using difference_type = std::ptrdiff_t;
            using pointer = const char* ;
            using reference = const char& ;

        private:
            pointer ptr_ = nullptr;
            friend class String;
            explicit ConstIterator(pointer ptr) : ptr_(ptr) {}

        public:
            ConstIterator() = default;
            // Conversion constructor from a non-const iterator to a const iterator
            ConstIterator(const Iterator& other) : ptr_(other.ptr_) {}

            reference operator*() const { return* ptr_; }
            pointer operator->() const { return ptr_; }

            ConstIterator& operator++()
            {
                ++ptr_;
                return *this;
            }
            ConstIterator operator++(int)
            {
                ConstIterator tmp = *this;
                ++ptr_;
                return tmp;
            }
            ConstIterator& operator--()
            {
                --ptr_;
                return *this;
            }
            ConstIterator operator--(int)
            {
                ConstIterator tmp = *this;
                --ptr_;
                return tmp;
            }

            ConstIterator& operator+=(difference_type n)
            {
                ptr_ += n;
                return *this;
            }
            ConstIterator& operator-=(difference_type n)
            {
                ptr_ -= n;
                return* this;
            }

            ConstIterator operator+(difference_type n) const { return ConstIterator(ptr_ + n); }
            ConstIterator operator-(difference_type n) const { return ConstIterator(ptr_ - n); }
            difference_type operator-(const ConstIterator& other) const { return ptr_ - other.ptr_; }

            reference operator[](difference_type n) const { return ptr_[n]; }

            friend bool operator==(const ConstIterator& lhs, const ConstIterator& rhs) noexcept { return lhs.ptr_ == rhs.ptr_; }
        };

        using iterator = Iterator;
        using const_iterator = ConstIterator;

    private:
        static constexpr size_type SSO_CAPACITY = 15;

        union
        {
            char sso_data_[SSO_CAPACITY + 1];
            pointer heap_data_;
        };

        size_type size_ = 0;
        size_type capacity_ = SSO_CAPACITY;
        Allocator<char> alloc_;

        [[nodiscard]] bool is_sso() const noexcept { return capacity_ <= SSO_CAPACITY; }
        [[nodiscard]] pointer data_ptr() noexcept { return is_sso() ? sso_data_ : heap_data_; }
        [[nodiscard]] const_pointer data_ptr() const noexcept { return is_sso() ? sso_data_ : heap_data_; }

        size_type internal_strlen(const char* s) const
        {
            size_type len = 0;
            while (s && s[len])
                ++len;
            return len;
        }

    public:
        // Iterators
        iterator begin() noexcept { return iterator(data_ptr()); }
        iterator end() noexcept { return iterator(data_ptr() + size_); }
        const_iterator begin() const noexcept { return const_iterator(data_ptr()); }
        const_iterator end() const noexcept { return const_iterator(data_ptr() + size_); }
        const_iterator cbegin() const noexcept { return const_iterator(data_ptr()); }
        const_iterator cend() const noexcept { return const_iterator(data_ptr() + size_); }

        // Constructors and Rule of Five
        String() noexcept;
        String(const char* s);
        String(size_type count, char ch);
        String(const String& other);
        String(String&& other) noexcept;
        ~String();

        String& operator=(const String& other);
        String& operator=(String&& other) noexcept;

        // Basic access
        [[nodiscard]] const char* c_str() const noexcept { return data_ptr(); }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

        reference operator[](size_type i) noexcept { return data_ptr()[i]; }
        const_reference operator[](size_type i) const noexcept { return data_ptr()[i]; }

        // Modifiers and operations (implementation placeholders)
        void clear() noexcept;
        void reserve(size_type new_capacity);
        void push_back(char ch);

        int compare(const String& other) const noexcept;

        friend bool operator==(const String& lhs, const String& rhs) noexcept;
        friend bool operator!=(const String& lhs, const String& rhs) noexcept;

        // Efficiently compare with C-strings (helps GoogleTest EXPECT_EQ)
        friend bool operator==(const String& lhs, const char* rhs) noexcept;

        friend std::ostream& operator<<(std::ostream& out, const String& str);
    };

} // namespace mystl