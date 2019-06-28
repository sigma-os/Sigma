#include <Sigma/arch/x86_64/drivers/cmos.h>

uint8_t x86_64::cmos::read(uint8_t reg){
    x86_64::io::outb(x86_64::cmos::select_reg, reg); // select register

    return x86_64::io::inb(x86_64::cmos::data_reg);
} 

void x86_64::cmos::write(uint8_t reg, uint8_t val){
    x86_64::io::outb(x86_64::cmos::select_reg, reg); // select register

    x86_64::io::outb(x86_64::cmos::data_reg, val);
} 

void x86_64::cmos::sleep_second(){
    uint8_t start_seconds = x86_64::cmos::read(x86_64::cmos::reg_seconds);
    bool done = false;
    while(!done){
        uint8_t current_seconds = x86_64::cmos::read(x86_64::cmos::reg_seconds);
        if(start_seconds == 59){
            if(current_seconds == 60) return;
            else if(current_seconds == 0) return;
        } else if(start_seconds == 60){
            if(current_seconds == 0) return;
        } else if(current_seconds == (start_seconds + 1)) return;
    }
}