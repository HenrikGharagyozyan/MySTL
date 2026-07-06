#pragma once

#include "utility.hpp"
#include "type_traits.hpp"
#include <cstddef>

namespace mystl 
{
    // ========================================================================
    // DEFAULT DELETE
    // ========================================================================

    template <typename T>
    struct default_delete 
    {
        constexpr default_delete() noexcept = default;

        template <typename U, typename = mystl::enable_if_t<mystl::is_convertible_v<U*, T*>>>
        default_delete(const default_delete<U>&) noexcept {}

        void operator()(T* ptr) const noexcept 
        {
            static_assert(sizeof(T) > 0, "mystl::default_delete: can't delete an incomplete type");
            delete ptr;
        }
    };

    // Specialization for arrays (unique_ptr<T[]>)
    template <typename T>
    struct default_delete<T[]> 
    {
        constexpr default_delete() noexcept = default;

        template <typename U, typename = mystl::enable_if_t<mystl::is_convertible_v<U(*)[], T(*)[]>>>
        default_delete(const default_delete<U[]>&) noexcept {}

        template <typename U, typename = mystl::enable_if_t<mystl::is_convertible_v<U(*)[], T(*)[]>>>
        void operator()(U* ptr) const noexcept 
        {
            static_assert(sizeof(U) > 0, "mystl::default_delete: can't delete an incomplete type");
            delete[] ptr;
        }
    };

    // ========================================================================
    // EBCO STORAGE (Empty Base Class Optimization)
    // ========================================================================
    namespace detail 
    {
        // Storage for pointer and deleter.
        // We use SFINAE to choose the optimal memory layout.
        
        template <typename Ptr, typename Deleter, bool IsEmpty = mystl::is_empty_v<Deleter> && !mystl::is_final_v<Deleter>>
        class compressed_ptr;

        // Variant 1: Deleter is empty (EBCO is applied) -> inherit from it (0 byte overhead)
        template <typename Ptr, typename Deleter>
        class compressed_ptr<Ptr, Deleter, true> : private Deleter 
        {
        public:
            Ptr ptr_;

            constexpr compressed_ptr() noexcept : Deleter(), ptr_(nullptr) {}
            constexpr compressed_ptr(Ptr p) noexcept : Deleter(), ptr_(p) {}
            constexpr compressed_ptr(Ptr p, const Deleter& d) noexcept : Deleter(d), ptr_(p) {}
            constexpr compressed_ptr(Ptr p, Deleter&& d) noexcept : Deleter(mystl::move(d)), ptr_(p) {}

            Deleter& get_deleter() noexcept { return *this; }
            const Deleter& get_deleter() const noexcept { return *this; }
        };

        // Variant 2: Deleter contains data -> store as regular field
        template <typename Ptr, typename Deleter>
        class compressed_ptr<Ptr, Deleter, false> 
        {
        public:
            Ptr ptr_;
            Deleter deleter_;

            constexpr compressed_ptr() noexcept : ptr_(nullptr), deleter_() {}
            constexpr compressed_ptr(Ptr p) noexcept : ptr_(p), deleter_() {}
            constexpr compressed_ptr(Ptr p, const Deleter& d) noexcept : ptr_(p), deleter_(d) {}
            constexpr compressed_ptr(Ptr p, Deleter&& d) noexcept : ptr_(p), deleter_(mystl::move(d)) {}

            Deleter& get_deleter() noexcept { return deleter_; }
            const Deleter& get_deleter() const noexcept { return deleter_; }
        };
    }

    // ========================================================================
    // UNIQUE_PTR PRIMARY TEMPLATE (Single Objects)
    // ========================================================================

    template <typename T, typename Deleter = mystl::default_delete<T>>
    class unique_ptr 
    {
    public:
        using pointer      = T*;
        using element_type = T;
        using deleter_type = Deleter;

    private:
        detail::compressed_ptr<pointer, deleter_type> storage_;

    public:
        constexpr unique_ptr() noexcept : storage_() {}
        constexpr unique_ptr(mystl::nullptr_t) noexcept : storage_() {}
        explicit unique_ptr(pointer p) noexcept : storage_(p) {}
        
        unique_ptr(pointer p, const deleter_type& d) noexcept : storage_(p, d) {}
        unique_ptr(pointer p, deleter_type&& d) noexcept : storage_(p, mystl::move(d)) {}

        // Move semantics
        unique_ptr(unique_ptr&& other) noexcept 
            : storage_(other.release(), mystl::forward<deleter_type>(other.get_deleter())) {}

        unique_ptr& operator=(unique_ptr&& other) noexcept 
        {
            reset(other.release());
            storage_.get_deleter() = mystl::forward<deleter_type>(other.get_deleter());
            return *this;
        }

        unique_ptr& operator=(mystl::nullptr_t) noexcept 
        {
            reset();
            return *this;
        }

        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        ~unique_ptr() noexcept 
        {
            if (storage_.ptr_ != nullptr) 
            {
                storage_.get_deleter()(storage_.ptr_);
            }
        }

        // Observers
        pointer get() const noexcept { return storage_.ptr_; }
        deleter_type& get_deleter() noexcept { return storage_.get_deleter(); }
        const deleter_type& get_deleter() const noexcept { return storage_.get_deleter(); }
        
        explicit operator bool() const noexcept { return storage_.ptr_ != nullptr; }

        // Modifiers
        pointer release() noexcept 
        {
            pointer p = storage_.ptr_;
            storage_.ptr_ = nullptr;
            return p;
        }

        void reset(pointer p = pointer()) noexcept 
        {
            pointer old_ptr = storage_.ptr_;
            storage_.ptr_ = p;
            if (old_ptr != nullptr) 
            {
                storage_.get_deleter()(old_ptr);
            }
        }

        void swap(unique_ptr& other) noexcept 
        {
            mystl::swap(storage_.ptr_, other.storage_.ptr_);
            mystl::swap(storage_.get_deleter(), other.storage_.get_deleter());
        }

        // Access to data
        mystl::add_lvalue_reference_t<T> operator*() const 
        {
            return *storage_.ptr_;
        }

        pointer operator->() const noexcept 
        {
            return storage_.ptr_;
        }
    };

    // ========================================================================
    // UNIQUE_PTR ARRAY SPECIALIZATION (unique_ptr<T[]>)
    // ========================================================================

    template <typename T, typename Deleter>
    class unique_ptr<T[], Deleter> 
    {
    public:
        using pointer      = T*;
        using element_type = T;
        using deleter_type = Deleter;

    private:
        detail::compressed_ptr<pointer, deleter_type> storage_;

    public:
        constexpr unique_ptr() noexcept : storage_() {}
        constexpr unique_ptr(mystl::nullptr_t) noexcept : storage_() {}
        
        // Array version requires an exact type match (disallows polymorphism)
        template <typename U, typename = mystl::enable_if_t<mystl::is_same_v<U*, pointer>>>
        explicit unique_ptr(U* p) noexcept : storage_(p) {}

        unique_ptr(pointer p, const deleter_type& d) noexcept : storage_(p, d) {}
        unique_ptr(pointer p, deleter_type&& d) noexcept : storage_(p, mystl::move(d)) {}

        unique_ptr(unique_ptr&& other) noexcept 
            : storage_(other.release(), mystl::forward<deleter_type>(other.get_deleter())) {}

        unique_ptr& operator=(unique_ptr&& other) noexcept 
        {
            reset(other.release());
            storage_.get_deleter() = mystl::forward<deleter_type>(other.get_deleter());
            return *this;
        }

        unique_ptr& operator=(mystl::nullptr_t) noexcept 
        {
            reset();
            return *this;
        }

        unique_ptr(const unique_ptr&) = delete;
        unique_ptr& operator=(const unique_ptr&) = delete;

        ~unique_ptr() noexcept 
        {
            if (storage_.ptr_ != nullptr) 
            {
                storage_.get_deleter()(storage_.ptr_);
            }
        }

        pointer get() const noexcept { return storage_.ptr_; }
        deleter_type& get_deleter() noexcept { return storage_.get_deleter(); }
        const deleter_type& get_deleter() const noexcept { return storage_.get_deleter(); }
        
        explicit operator bool() const noexcept { return storage_.ptr_ != nullptr; }

        pointer release() noexcept 
        {
            pointer p = storage_.ptr_;
            storage_.ptr_ = nullptr;
            return p;
        }

        template <typename U, typename = mystl::enable_if_t<mystl::is_same_v<U*, pointer>>>
        void reset(U* p = pointer()) noexcept 
        {
            pointer old_ptr = storage_.ptr_;
            storage_.ptr_ = p;
            if (old_ptr != nullptr) 
            {
                storage_.get_deleter()(old_ptr);
            }
        }

        void reset(mystl::nullptr_t = nullptr) noexcept 
        {
            reset(pointer());
        }

        void swap(unique_ptr& other) noexcept 
        {
            mystl::swap(storage_.ptr_, other.storage_.ptr_);
            mystl::swap(storage_.get_deleter(), other.storage_.get_deleter());
        }

        // Index operator instead of dereference
        mystl::add_lvalue_reference_t<T> operator[](std::size_t i) const 
        {
            return storage_.ptr_[i];
        }
    };

    // ========================================================================
    // MAKE_UNIQUE
    // ========================================================================

    // For single objects
    template <typename T, typename... Args>
    mystl::enable_if_t<!mystl::is_array_v<T>, unique_ptr<T>>
    make_unique(Args&&... args) 
    {
        return unique_ptr<T>(new T(mystl::forward<Args>(args)...));
    }

    // For arrays of unknown bound (e.g. make_unique<int[]>(5))
    template <typename T>
    mystl::enable_if_t<mystl::is_unbounded_array_v<T>, unique_ptr<T>>
    make_unique(std::size_t n) 
    {
        return unique_ptr<T>(new mystl::remove_extent_t<T>[n]());
    }

    // For arrays of known bound - forbidden by the standard (e.g. make_unique<int[5]>())
    template <typename T, typename... Args>
    mystl::enable_if_t<mystl::is_bounded_array_v<T>> make_unique(Args&&...) = delete;

    // Global swap
    template <typename T, typename D>
    void swap(unique_ptr<T, D>& lhs, unique_ptr<T, D>& rhs) noexcept 
    {
        lhs.swap(rhs);
    }

} // namespace mystl