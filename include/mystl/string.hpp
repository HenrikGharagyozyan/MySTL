#pragma once

#include "utility.hpp"
#include "iterator.hpp"
#include "memory.hpp"
#include "allocator.hpp"
#include "algorithm.hpp"

#include <cstddef>
#include <iostream>
#include <stdexcept>

namespace mystl
{
    class String
    {
    public:
        using value_type             = char;
        using allocator_type         = mystl::Allocator<char>;
        using allocator_traits_type  = mystl::allocator_traits<allocator_type>;
        using size_type              = typename allocator_traits_type::size_type;
        using difference_type        = std::ptrdiff_t;
        using reference              = value_type&;
        using const_reference        = const value_type&;
        using pointer                = typename allocator_traits_type::pointer;
        using const_pointer          = const value_type*;

        // ========================================================================
        // ИТЕРАТОРЫ (Сырые указатели - самые быстрые Random Access итераторы)
        // ========================================================================
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = mystl::reverse_iterator<iterator>;
        using const_reverse_iterator = mystl::reverse_iterator<const_iterator>;

    private:
        static constexpr size_type SSO_CAPACITY = 15;

        union
        {
            char sso_data_[SSO_CAPACITY + 1];
            pointer heap_data_;
        };

        size_type size_ = 0;
        size_type capacity_ = SSO_CAPACITY;
        [[no_unique_address]] allocator_type alloc_;

        [[nodiscard]] bool is_sso() const noexcept { return capacity_ <= SSO_CAPACITY; }
        [[nodiscard]] pointer data_ptr() noexcept { return is_sso() ? sso_data_ : heap_data_; }
        [[nodiscard]] const_pointer data_ptr() const noexcept { return is_sso() ? sso_data_ : heap_data_; }

        static size_type internal_strlen(const char* s) noexcept
        {
            size_type len = 0;
            if (s)
            {
                while (s[len] != '\0') ++len;
            }
            return len;
        }

    public:
        // ========================================================================
        // ИТЕРАТОРЫ
        // ========================================================================
        iterator begin() noexcept { return data_ptr(); }
        iterator end() noexcept { return data_ptr() + size_; }
        const_iterator begin() const noexcept { return data_ptr(); }
        const_iterator end() const noexcept { return data_ptr() + size_; }
        const_iterator cbegin() const noexcept { return data_ptr(); }
        const_iterator cend() const noexcept { return data_ptr() + size_; }

        reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
        reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
        const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
        const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

        // ========================================================================
        // КОНСТРУКТОРЫ И RULE OF FIVE
        // ========================================================================
        String() noexcept
            : size_(0)
            , capacity_(SSO_CAPACITY)
        {
            sso_data_[0] = '\0';
        }

        String(const char* s)
        {
            size_ = internal_strlen(s);
            if (size_ <= SSO_CAPACITY)
            {
                capacity_ = SSO_CAPACITY;
                mystl::copy(s, s + size_, sso_data_);
                sso_data_[size_] = '\0';
            }
            else
            {
                capacity_ = size_;
                heap_data_ = allocator_traits_type::allocate(alloc_, capacity_ + 1);
                mystl::copy(s, s + size_, heap_data_);
                heap_data_[size_] = '\0';
            }
        }

        String(size_type count, char ch)
            : size_(count)
        {
            if (size_ <= SSO_CAPACITY)
            {
                capacity_ = SSO_CAPACITY;
                mystl::fill(sso_data_, sso_data_ + size_, ch);
                sso_data_[size_] = '\0';
            }
            else
            {
                capacity_ = size_;
                heap_data_ = allocator_traits_type::allocate(alloc_, capacity_ + 1);
                mystl::fill(heap_data_, heap_data_ + size_, ch);
                heap_data_[size_] = '\0';
            }
        }

        String(const String& other)
            : size_(other.size_)
            , capacity_(other.capacity_)
        {
            if (other.is_sso())
            {
                mystl::copy(other.sso_data_, other.sso_data_ + size_ + 1, sso_data_);
            }
            else
            {
                heap_data_ = allocator_traits_type::allocate(alloc_, capacity_ + 1);
                mystl::copy(other.heap_data_, other.heap_data_ + size_ + 1, heap_data_);
            }
        }

        String(String&& other) noexcept
            : size_(other.size_)
            , capacity_(other.capacity_)
        {
            if (other.is_sso())
            {
                mystl::copy(other.sso_data_, other.sso_data_ + size_ + 1, sso_data_);
            }
            else
            {
                heap_data_ = other.heap_data_;
            }

            // Переводим исходный объект в дефолтное состояние SSO
            other.size_ = 0;
            other.capacity_ = SSO_CAPACITY;
            other.sso_data_[0] = '\0';
        }

        ~String()
        {
            if (!is_sso() && heap_data_)
            {
                allocator_traits_type::deallocate(alloc_, heap_data_, capacity_ + 1);
            }
        }

        String& operator=(const String& other)
        {
            if (this == &other)
                return *this;

            if (!is_sso() && heap_data_)
            {
                allocator_traits_type::deallocate(alloc_, heap_data_, capacity_ + 1);
            }

            size_ = other.size_;
            capacity_ = other.capacity_;

            if (other.is_sso())
            {
                mystl::copy(other.sso_data_, other.sso_data_ + size_ + 1, sso_data_);
            }
            else
            {
                heap_data_ = allocator_traits_type::allocate(alloc_, capacity_ + 1);
                mystl::copy(other.heap_data_, other.heap_data_ + size_ + 1, heap_data_);
            }
            return *this;
        }

        String& operator=(String&& other) noexcept
        {
            if (this == &other)
                return *this;

            if (!is_sso() && heap_data_)
            {
                allocator_traits_type::deallocate(alloc_, heap_data_, capacity_ + 1);
            }

            size_ = other.size_;
            capacity_ = other.capacity_;

            if (other.is_sso())
            {
                mystl::copy(other.sso_data_, other.sso_data_ + size_ + 1, sso_data_);
            }
            else
            {
                heap_data_ = other.heap_data_;
            }

            other.size_ = 0;
            other.capacity_ = SSO_CAPACITY;
            other.sso_data_[0] = '\0';

            return *this;
        }

        // ========================================================================
        // ДОСТУП К ДАННЫМ
        // ========================================================================
        [[nodiscard]] const char* c_str() const noexcept { return data_ptr(); }
        [[nodiscard]] size_type size() const noexcept { return size_; }
        [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
        [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

        reference operator[](size_type i) noexcept { return data_ptr()[i]; }
        const_reference operator[](size_type i) const noexcept { return data_ptr()[i]; }

        // ========================================================================
        // МОДИФИКАТОРЫ
        // ========================================================================
        void clear() noexcept
        {
            size_ = 0;
            data_ptr()[0] = '\0';
        }

        void reserve(size_type new_capacity)
        {
            if (new_capacity <= capacity_)
                return;

            pointer new_data = allocator_traits_type::allocate(alloc_, new_capacity + 1);
            pointer old_data = data_ptr();

            mystl::copy(old_data, old_data + size_ + 1, new_data);

            if (!is_sso())
            {
                allocator_traits_type::deallocate(alloc_, heap_data_, capacity_ + 1);
            }

            heap_data_ = new_data;
            capacity_ = new_capacity;
        }

        void push_back(char ch)
        {
            if (size_ == capacity_)
                reserve(capacity_ == 0 ? SSO_CAPACITY : capacity_ * 2);

            pointer data = data_ptr();
            data[size_] = ch;
            data[size_ + 1] = '\0';
            ++size_;
        }

        int compare(const String& other) const noexcept
        {
            size_type min_size = mystl::min(size_, other.size_);
            const char* data1 = data_ptr();
            const char* data2 = other.data_ptr();

            for (size_type i = 0; i < min_size; ++i)
            {
                if (data1[i] < data2[i]) return -1;
                if (data1[i] > data2[i]) return 1;
            }

            if (size_ < other.size_) return -1;
            if (size_ > other.size_) return 1;
            return 0;
        }

        // ========================================================================
        // ДРУЖЕСТВЕННЫЕ ФУНКЦИИ (СРАВНЕНИЯ И IO)
        // ========================================================================
        friend bool operator==(const String& lhs, const String& rhs) noexcept
        {
            if (lhs.size_ != rhs.size_) return false;
            return mystl::equal(lhs.begin(), lhs.end(), rhs.begin());
        }

        friend bool operator!=(const String& lhs, const String& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend bool operator<(const String& lhs, const String& rhs) noexcept
        {
            return mystl::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator>(const String& lhs, const String& rhs) noexcept
        {
            return rhs < lhs;
        }

        friend bool operator<=(const String& lhs, const String& rhs) noexcept
        {
            return !(rhs < lhs);
        }

        friend bool operator>=(const String& lhs, const String& rhs) noexcept
        {
            return !(lhs < rhs);
        }

        friend bool operator==(const String& lhs, const char* rhs) noexcept
        {
            if (!rhs) return false;
            size_type len = internal_strlen(rhs);
            if (lhs.size_ != len) return false;
            return mystl::equal(lhs.begin(), lhs.end(), rhs);
        }

        friend std::ostream& operator<<(std::ostream& out, const String& str)
        {
            out << str.c_str();
            return out;
        }
    };

} // namespace mystl