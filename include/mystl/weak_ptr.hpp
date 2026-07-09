#pragma once

#include "shared_ptr.hpp" // shared_ptr и control_block

namespace mystl 
{

    template <typename T>
    class weak_ptr 
    {
    private:
        T*                  ptr_{nullptr};
        control_block_base* cb_{nullptr};

        template <typename U> friend class weak_ptr;
        template <typename U> friend class shared_ptr;

    public:
        constexpr weak_ptr() noexcept = default;

        template <typename Y>
<<<<<<< HEAD
        weak_ptr(const shared_ptr<Y>& shared) noexcept 
=======
        weak_ptr(const shared_ptr<Y>& shared) noexcept  
>>>>>>> 1660d6f (Add roadmap)
            : ptr_(shared.ptr_), cb_(shared.cb_) 
        {
            if (cb_) 
                cb_->add_weak();
        }

        weak_ptr(const weak_ptr& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_) 
        {
            if (cb_) 
                cb_->add_weak();
        }

        weak_ptr(weak_ptr&& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_) 
        {
            other.ptr_ = nullptr;
            other.cb_  = nullptr;
        }

        ~weak_ptr() 
        {
            if (cb_) 
                cb_->release_weak();
        }

        weak_ptr& operator=(const weak_ptr& r) noexcept 
        {
            weak_ptr(r).swap(*this);
            return *this;
        }

        weak_ptr& operator=(weak_ptr&& r) noexcept 
        {
            weak_ptr(mystl::move(r)).swap(*this);
            return *this;
        }

        template <typename Y>
        weak_ptr& operator=(const shared_ptr<Y>& r) noexcept 
        {
            weak_ptr(r).swap(*this);
            return *this;
        }

        void swap(weak_ptr& other) noexcept 
        {
            mystl::swap(ptr_, other.ptr_);
            mystl::swap(cb_, other.cb_);
        }

        void reset() noexcept 
        {
            weak_ptr().swap(*this);
        }

        // Observers
        long use_count() const noexcept 
        {
            return cb_ ? cb_->shared_count.load(std::memory_order_relaxed) : 0;
        }

        bool expired() const noexcept 
        {
            return use_count() == 0;
        }

        shared_ptr<T> lock() const noexcept 
        {
            if (cb_ && cb_->lock()) 
            {
                // Если lock() вернул true, значит мы атомарно увеличили shared_count
                // Теперь мы можем безопасно создать shared_ptr, используя приватный конструктор
                return shared_ptr<T>(cb_, ptr_);
            }
            return shared_ptr<T>(); // Объект уже мертв, возвращаем пустой shared_ptr
        }
    };

} // namespace mystl