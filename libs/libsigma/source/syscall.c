#include <libsigma/syscall.h>
#include "common.h"

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall0(uint64_t number){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number));
    return ret;
}

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall1(uint64_t number, uint64_t arg1){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number), "b"(arg1));
    return ret;
}

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall2(uint64_t number, uint64_t arg1, uint64_t arg2){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number), "b"(arg1), "c"(arg2));
    return ret;
}

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall3(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number), "b"(arg1), "c"(arg2), "d" (arg3));
    return ret;
}

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall4(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number), "b"(arg1), "c"(arg2), "d" (arg3), "S"(arg4));
    return ret;
}

ALWAYSINLINE_ATTRIBUTE
inline uint64_t libsigma_syscall5(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5){
    uint64_t ret = 0;
    asm("int $249" : "=a"(ret): "a"(number), "b"(arg1), "c"(arg2), "d" (arg3), "S"(arg4), "D"(arg5));
    return ret;
}
