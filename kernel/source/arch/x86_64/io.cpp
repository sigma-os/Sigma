#include <Sigma/arch/x86_64/io.h>

uint8_t x86_64::io::inb(uint16_t port){
    uint8_t ret;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void x86_64::io::outb(uint16_t port, uint8_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}

uint16_t x86_64::io::inw(uint16_t port){
    uint16_t ret;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void x86_64::io::outw(uint16_t port, uint16_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t x86_64::io::ind(uint16_t port){
    uint32_t ret;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void x86_64::io::outd(uint16_t port, uint32_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}