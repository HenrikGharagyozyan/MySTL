#pragma once

#include "cstddef.hpp"
#include "utility.hpp"
#include <atomic>


namespace mystl 
{

    template <typename T> class weak_ptr;
    template <typename T> class shared_ptr;

    // ========================================================================
    // CONTROL BLOCK 
    // ========================================================================

    // Contains only atomic counters and virtual methods for polymorphic deletion.
    struct control_block_base 
    {
        std::atomic<long> shared_count; 
        std::atomic<long> weak_count;   

        // Default constructor: 1 owner, 0 watchers
        constexpr control_block_base() noexcept 
            : shared_count(1)
            , weak_count(1) 
        {
        }

        virtual ~control_block_base() = default;

        // Destroys the object itself (called when shared_count == 0)
        virtual void dispose() noexcept = 0;

        // Destroys the control block itself (called when shared_count == 0 and weak_count == 0)
        virtual void destroy() noexcept = 0;

        // Thread-safe increment of counter
        void add_shared() noexcept 
        {
            // memory_order_relaxed means we only care about the fact of incrementing,
            // without strict synchronization of other data in memory
            shared_count.fetch_add(1, std::memory_order_relaxed);
        }

        // Thread-safe decrement of counter
        // Returns true if counter reaches zero
        void release_shared() noexcept 
        {
            // memory_order_acq_rel ensures that all memory operations are completed before we delete the object
            if (shared_count.fetch_sub(1, std::memory_order_acq_rel) == 1) 
            {
                dispose(); // Delete the object
                release_weak();
            }
        }

        // Потокобезопасное увеличение счетчика weak_ptr
        void add_weak() noexcept 
        {
            weak_count.fetch_add(1, std::memory_order_relaxed);
        }

        // Потокобезопасное уменьшение счетчика weak_ptr
        void release_weak() noexcept 
        {
            if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) 
                destroy(); // Если это был последний weak_ptr и shared_ptr тоже нет - удаляем блок
        }

        // Потокобезопасная попытка увеличить счетчик shared_count, если он не равен 0.
        // Используется в weak_ptr::lock().
        bool lock() noexcept 
        {
            // Пытаемся атомарно увеличить shared_count, если он > 0
            long count = shared_count.load(std::memory_order_relaxed);
            while (count != 0) 
            {
                if (shared_count.compare_exchange_weak(count, count + 1,
                        std::memory_order_acquire, std::memory_order_relaxed)) 
                {
                    return true;
                }
            }
            return false;
        }
    };

    // Concrete implementation of control block for a raw pointer
    template <typename T>
    class control_block_ptr final : public control_block_base 
    {
    private:
        T* ptr_; // Raw pointer to the managed object

    public:
        explicit control_block_ptr(T* p) : ptr_(p) {}

        void dispose() noexcept override 
        {
            delete ptr_;
            // We don't null ptr_ because the block will be destroyed soon anyway,
            // and this saves CPU cycles
        }

        void destroy() noexcept override 
        {
            delete this; // Block deletes itself from memory
        }
    };

    // Implementation of a control block that stores the object directly INSIDE itself.
    // Used for make_shared.
    template <typename T>
    class control_block_obj final : public control_block_base 
    {
    private:
        // Reserve raw memory of the required size and with alignment of type T
        alignas(T) char storage_[sizeof(T)];

    public:
        // Block constructor accepts any arguments and constructs the object via placement new
        template <typename... Args>
        explicit control_block_obj(Args&&... args) 
        {
            ::new (static_cast<void*>(storage_)) T(mystl::forward<Args>(args)...);
        }

        void dispose() noexcept override 
        {
            // Explicitly call the object's destructor, but don't free the memory!
            get_ptr()->~T();
        }

        void destroy() noexcept override 
        {
            // Free the entire control block's memory
            delete this;
        }

        T* get_ptr() noexcept 
        {
            return reinterpret_cast<T*>(storage_);
        }
    };


    // ========================================================================
    // SHARED_PTR
    // ========================================================================

    template <typename T>
    class shared_ptr 
    {
    private:
        T*                  ptr_{nullptr}; // Pointer to the object
        control_block_base* cb_{nullptr};  // Pointer to the control block

        // Friend class to enable conversion of shared_ptr<Derived> to shared_ptr<Base>
        template <typename U> friend class shared_ptr;
        template <typename U> friend class weak_ptr;

    public:
        // Default constructor
        constexpr shared_ptr() noexcept = default;
        constexpr shared_ptr(mystl::nullptr_t) noexcept : shared_ptr() {}

        template <typename Y>
        explicit shared_ptr(Y* p) 
        {
            if (p) 
            {
                ptr_ = p;
                cb_  = new control_block_ptr<Y>(p);
            }
        }

        shared_ptr(const shared_ptr& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_)
        {
            if (cb_) 
            {
                cb_->add_shared();
            }
        }

        // Copy constructor with type conversion (Derived* -> Base*)
        template <typename Y>
        shared_ptr(const shared_ptr<Y>& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_)
        {
            if (cb_) 
            {
                cb_->add_shared();
            }
        }

        shared_ptr(shared_ptr&& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_)
        {
            other.ptr_ = nullptr;
            other.cb_  = nullptr;
        }

        template <typename Y>
        shared_ptr(shared_ptr<Y>&& other) noexcept 
            : ptr_(other.ptr_), cb_(other.cb_)
        {
            other.ptr_ = nullptr;
            other.cb_  = nullptr;
        }

        template <typename Y>
        explicit shared_ptr(const weak_ptr<Y>& weak) 
            : ptr_(weak.ptr_), cb_(weak.cb_)
        {
            if (cb_ && !cb_->lock()) 
            {
                ptr_ = nullptr;
                cb_ = nullptr;
            }
        }

        ~shared_ptr() 
        {
            if (cb_) 
                cb_->release_shared();
        }

        shared_ptr& operator=(const shared_ptr& r) noexcept 
        {
            // Use Copy-and-Swap idiom for safe assignment
            shared_ptr(r).swap(*this);
            return *this;
        }

        shared_ptr& operator=(shared_ptr&& r) noexcept 
        {
            shared_ptr(mystl::move(r)).swap(*this);
            return *this;
        }

        void swap(shared_ptr& other) noexcept 
        {
            mystl::swap(ptr_, other.ptr_);
            mystl::swap(cb_, other.cb_);
        }

        void reset() noexcept 
        {
            shared_ptr().swap(*this);
        }

        // Observers
        T* get() const noexcept { return ptr_; }
        
        long use_count() const noexcept 
        {
            return cb_ ? cb_->shared_count.load(std::memory_order_relaxed) : 0;
        }

        bool unique() const noexcept { return use_count() == 1; }

        explicit operator bool() const noexcept { return ptr_ != nullptr; }

        T& operator*() const noexcept { return *ptr_; }
        T* operator->() const noexcept { return ptr_; }

    private:
        explicit shared_ptr(control_block_base* cb, T* ptr) noexcept 
            : ptr_(ptr), cb_(cb) 
        {
        }

        template <typename U, typename... Args>
        friend shared_ptr<U> make_shared(Args&&... args);
    };

    // Global swap for mystl::shared_ptr
    template <typename T>
    void swap(shared_ptr<T>& a, shared_ptr<T>& b) noexcept 
    {
        a.swap(b);
    }

    template <typename T, typename... Args>
    shared_ptr<T> make_shared(Args&&... args) 
    {
        auto* cb = new control_block_obj<T>(mystl::forward<Args>(args)...);
        
        return shared_ptr<T>(cb, cb->get_ptr());
    }

} // namespace mystl