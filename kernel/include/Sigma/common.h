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

using std::size_t;

#include <Sigma/bitops.h>
#include <Sigma/panic.h>
#include <Sigma/compiler.h>

#define DEBUG
#define LOG_SYSCALLS


#define KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE 0xffff800000000000
#define KERNEL_VBASE 0xffffffff80000000
#define KERNEL_PBASE 0x0000000000100000

#define C_LINKAGE extern "C"

#define PANIC(message) (Sigma::panic::panic_m(message, __FILE__, __func__, __LINE__))

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

namespace common {
	constexpr uint64_t div_ceil(uint64_t val, uint64_t div) {
		return (val + div - 1) / div;
	}

	constexpr bool is_canonical(uint64_t addr){
		return ((addr <= 0x00007fffffffffff) || ((addr >= 0xffff800000000000) && (addr <= 0xffffffffffffffff)));
	}
} // namespace common


#endif