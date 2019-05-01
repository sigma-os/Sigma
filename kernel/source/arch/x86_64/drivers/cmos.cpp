#include <Sigma/arch/x86_64/drivers/cmos.h>

uint8_t x86_64::cmos::read(uint8_t reg){
    x86_64::io::outb(x86_64::cmos::select_reg, reg); // select register

    return x86_64::io::inb(x86_64::cmos::data_reg);
} 

void x86_64::cmos::write(uint8_t reg, uint8_t val){
    x86_64::io::outb(x86_64::cmos::select_reg, reg); // select register

    x86_64::io::outb(x86_64::cmos::data_reg, val);
} 