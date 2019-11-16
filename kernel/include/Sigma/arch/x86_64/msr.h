#ifndef SIGMA_KERNEL_X86_64_MSR
#define SIGMA_KERNEL_X86_64_MSR

#include <Sigma/common.h>

namespace x86_64::msr
{
    constexpr uint32_t ia32_pat = 0x277;
    constexpr uint32_t ia32_tme_capability = 0x981;
    constexpr uint32_t ia32_tme_activate = 0x982;
    constexpr uint32_t ia32_tme_exclude_mask = 0x983;
    constexpr uint32_t ia32_tme_exclude_base = 0x984;
    constexpr uint32_t ia32_efer = 0xC0000080;
    constexpr uint32_t apic_base = 0x0000001b;
    constexpr uint32_t fs_base =  0xC0000100;
    constexpr uint32_t gs_base =  0xC0000101;
    constexpr uint32_t kernelgs_base =  0xC0000102;

    uint64_t read(uint32_t msr);
    void write(uint32_t msr, uint64_t val);
} // x86_64::msr


#endif