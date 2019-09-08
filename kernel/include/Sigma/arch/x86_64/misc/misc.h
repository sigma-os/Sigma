#ifndef SIGMA_ARCH_X86_64_CPUID
#define SIGMA_ARCH_X86_64_CPUID

#include <Sigma/common.h>
#include <cpuid.h>

namespace x86_64
{
    bool cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);
    bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);
    uint64_t read_tsc();

    C_LINKAGE uint64_t read_xcr(uint32_t xcr);
    C_LINKAGE void write_xcr(uint32_t xcr, uint64_t value);
} // namespace x86_64


#endif