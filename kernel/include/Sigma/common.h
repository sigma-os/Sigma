#ifndef SIGMA_KERNEL_COMMON
#define SIGMA_KERNEL_COMMON

#include <cstdint>
#include <cstddef>
#include <new>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

using std::uintptr_t;
using std::size_t;

#include <Sigma/bitops.h>
#include <Sigma/panic.h>
#include <Sigma/compiler.h>



#define PANIC(message) (misc::panic::panic_m(message, SIGMA_FUNCTION_NAME, std::experimental::source_location::current()))

#define ASSERT(condition) do { \
				if(!(condition)){ \
                    PANIC("Assertion Failed, condition: " #condition); \
				} \
			} while(0);

#include <Sigma/misc.h>


// Things to log debug
#define DEBUG
//#define LOG_SYSCALLS


// General defines
#define KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE 0xffff800000000000
#define KERNEL_VBASE 0xffffffff80000000
#define KERNEL_PBASE 0x0000000000100000

#define C_LINKAGE extern "C"

#define FUNCTION_CALL_ONCE() ({ \
    static bool called = false; \
    if(!called) called = true; \
    else PANIC("Tried to reenter call-once function"); \
    })


#define ARCH_X86_64 // TODO: Actually separate archs
//#define ARCH_ARM

#define ALIGN_DOWN(n, a) (((uint64_t)n) & ~((a) - 1ul))
#define ALIGN_UP(n, a) ALIGN_DOWN(((uint64_t)n) + (a) - 1ul, (a))

using tid_t = uint64_t;

#endif