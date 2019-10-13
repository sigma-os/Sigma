#ifndef SIGMA_KERNEL_SPINLOCK
#define SIGMA_KERNEL_SPINLOCK

#include <Sigma/common.h>

namespace x86_64::spinlock {
    struct mutex;

    C_LINKAGE void acquire(mutex* lock);
    C_LINKAGE void release(mutex* lock);

    struct mutex {
        mutex(): _lock(0) {}
        
        void lock(){
            x86_64::spinlock::acquire(this);
        }

        void unlock(){
            x86_64::spinlock::release(this);
        }

        private:
        uint16_t _lock;
    };
}

#endif