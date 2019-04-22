#ifndef SIGMA_KERNEL_SPINLOCK
#define SIGMA_KERNEL_SPINLOCK

#include <Sigma/common.h>

namespace x86_64::spinlock {
    struct mutex {
        mutex(): lock(0) {}
        uint16_t lock;
    };

    C_LINKAGE void acquire(mutex* lock);
    C_LINKAGE void release(mutex* lock);
}

#endif