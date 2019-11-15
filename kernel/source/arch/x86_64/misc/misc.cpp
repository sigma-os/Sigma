#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/smp/cpu.h>
#include <klibcxx/mutex.hpp>
#include <klibc/stdio.h>

bool x86_64::cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d){
    return __get_cpuid(leaf, &eax, &ebx, &c, &d);
}

bool x86_64::cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d){
    return __get_cpuid_count(leaf, subleaf, &eax, &ebx, &c, &d);
}

uint64_t x86_64::read_tsc(){
    uint64_t tsc_low = 0;
    uint64_t tsc_high = 0;
    asm ("rdtsc" : "=a"(tsc_low), "=d"(tsc_high));
    return (tsc_low | (tsc_high << 32));
}