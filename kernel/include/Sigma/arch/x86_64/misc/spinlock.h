#ifndef SIGMA_KERNEL_SPINLOCK
#define SIGMA_KERNEL_SPINLOCK

#include <Sigma/common.h>

namespace x86_64::spinlock {
    struct mutex;

    C_LINKAGE void acquire(uint16_t* lock);
    C_LINKAGE void release(uint16_t* lock);
    C_LINKAGE bool try_acquire(uint16_t* lock);

    struct mutex {
        constexpr mutex(): _lock(0) {}
        
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
    };
}

#endif