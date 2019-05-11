#ifndef SIGMA_KERNEL_SMP_CPU
#define SIGMA_KERNEL_SMP_CPU

#include <Sigma/common.h>

namespace smp::cpu
{
    struct entry {
        public:
        entry() = default;
        uint64_t test;
    };
} // smp::cpu


#endif