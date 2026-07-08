#pragma once

#include "cstddef.hpp"
#include "utility.hpp"
#include <atomic>


namespace mystl 
{

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
            , weak_count(0) 
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
        bool release_shared() noexcept 
        {
            // memory_order_acq_rel ensures that all memory operations are completed before we delete the object
            if (shared_count.fetch_sub(1, std::memory_order_acq_rel) == 1) 
            {
                dispose(); // Delete the object
                
                // weak_count stores +1 "virtual" reference as long as at least one shared_ptr is alive.
                // Therefore, when shared_ptr are exhausted, we subtract this virtual reference.
                if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 0) 
                {
                    destroy(); // Delete the block itself
                }
                return true;
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

} // namespace mystl