#ifndef SIGMA_KERNEL_COMMON
#define SIGMA_KERNEL_COMMON

#include <stdint.h>
#include <stddef.h>

#include <Sigma/bitops.h>
#include <Sigma/panic.h>

// normally would be in <new> but that doesnt exist here
inline void *operator new(size_t, void *p)     throw() { return p; } 
inline void *operator new[](size_t, void *p)   throw() { return p; }
inline void  operator delete  (void *, void *) throw() { };
inline void  operator delete[](void *, void *) throw() { };

#define DEBUG


#define KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE 0xffff800000000000
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

#define ALIGN_DOWN(val, align) (((uint64_t)(val)) & ~((align) - 1))
#define ALIGN_UP(val, align) (ALIGN_DOWN((((uint64_t)(val)) + (align) - 1), (align)))

#define IS_CANONICAL(addr) ((((addr) <= 0x00007fffffffffff)) || (((addr) >= 0xffff800000000000) && ((addr) <= 0xffffffffffffffff)))

#endif