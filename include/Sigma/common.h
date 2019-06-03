#ifndef SIGMA_KERNEL_COMMON
#define SIGMA_KERNEL_COMMON

#include <stdint.h>
#include <stddef.h>

#include <Sigma/bitops.h>

#include <Sigma/panic.h>

#define DEBUG

#define KERNEL_VBASE 0xffffffff80000000
#define KERNEL_PBASE 0x0000000000100000

#define C_LINKAGE extern "C"

#define UNUSED(x) ((void)((x)))

#define PANIC(message) (Sigma::panic::panic_m(message, __FILE__, __func__, __LINE__))

#define FUNCTION_CALL_ONCE() ({ \
    static bool called = false; \
    if(!called) called = true; \
    else PANIC("Tried to reenter call-once function"); \
    })


#define ARCH_X86_64 // get the possibility of multiple platforms working
//#define ARCH_ARM

#endif