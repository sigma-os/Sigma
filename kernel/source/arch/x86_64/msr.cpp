#include <Sigma/arch/x86_64/msr.h>

uint64_t x86_64::msr::read(uint32_t msr){
    uint64_t val_low = 0;
    uint64_t val_high = 0;
    asm ("rdmsr" : "=a"(val_low), "=d"(val_high) : "c"(msr));
    return (val_low | (val_high << 32));
}

void x86_64::msr::write(uint32_t msr, uint64_t val){
    uint32_t val_low = (val & 0xFFFFFFFF);
    uint32_t val_high = ((val >> 32) & 0xFFFFFFFF);
    asm("wrmsr" : : "a"(val_low), "d"(val_high), "c"(msr));
}