#ifndef SIGMA_KERNEL_SPINLOCK
#define SIGMA_KERNEL_SPINLOCK

#include <Sigma/common.h>

namespace x86_64::spinlock {
    C_LINKAGE void acquire(uint16_t* lock);
    C_LINKAGE void release(uint16_t* lock);
    C_LINKAGE bool try_acquire(uint16_t* lock);

    struct mutex {
        constexpr mutex() noexcept : _lock(0) {}
        
        void lock(){
            x86_64::spinlock::acquire(&this->_lock);
        }

        void unlock(){
            x86_64::spinlock::release(&this->_lock);
        }

        bool try_lock(){
            return x86_64::spinlock::try_acquire(&this->_lock);
        }

        private:
        uint16_t _lock;
    struct irq_lock {
        constexpr irq_lock() noexcept: _count{0} {}

        irq_lock(const irq_lock &) = delete;
	    irq_lock &operator= (const irq_lock &) = delete;

        void lock(MAYBE_UNUSED_ATTRIBUTE KLIBCXX_NAMESPACE_NAME::experimental::source_location src = KLIBCXX_NAMESPACE_NAME::experimental::source_location::current()){
            auto c = _count.load(std::memory_order_acquire);
            if(c == 0){
                uint64_t rflags = 0;
                asm("pushf; pop %0" : "=r"(rflags));

                if(rflags & (1 << 9)){
                    asm volatile("cli" : : : "memory");
                    _count.store(old_irq_flag | 1, std::memory_order_relaxed);
                } else {
                    _count.store(1, std::memory_order_relaxed);
                }
            } else {
                ASSERT(c & ~old_irq_flag);
                _count.store(c + 1, std::memory_order_release);
            }   
        }

        void unlock(){
            auto c = _count.load(std::memory_order_relaxed);
            ASSERT(c & ~old_irq_flag);
            if((c & ~old_irq_flag) == 1){
                _count.store(0, std::memory_order_release);
                if(c & old_irq_flag)
                    asm volatile ("sti" : : : "memory");
            } else {
                _count.store(c - 1, std::memory_order_release);
            }
        }

        private:
        std::atomic<uint64_t> _count;

        constexpr static uint64_t old_irq_flag = (1ull << 63);
    };
}

#endif