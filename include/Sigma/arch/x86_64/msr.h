#ifndef SIGMA_KERNEL_X86_64_MSR
#define SIGMA_KERNEL_X86_64_MSR

#include <Sigma/common.h>

namespace x86_64::msr
{
    constexpr uint32_t ia32_efer = 0xC0000080;
    constexpr uint32_t apic_base = 0x0000001b;

    uint64_t read(uint32_t msr);
    void write(uint32_t msr, uint64_t val);
} // x86_64::msr


#endif