#pragma once

#include <stdint.h>

static inline uint8_t inb(uint16_t port){
    uint8_t ret = 0;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint16_t inw(uint16_t port){
    uint16_t ret = 0;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t ind(uint16_t port){
    uint32_t ret = 0;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outd(uint16_t port, uint32_t value){
    asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
}