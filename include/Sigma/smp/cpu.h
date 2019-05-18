#ifndef SIGMA_KERNEL_SMP_CPU
#define SIGMA_KERNEL_SMP_CPU

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/drivers/apic.h>

namespace smp::cpu
{
    struct entry {
        public:
        entry() = default;
        x86_64::apic::lapic lapic;
    };
} // smp::cpu


#endif