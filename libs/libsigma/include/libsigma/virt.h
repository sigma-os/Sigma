#ifndef LIBSIGMA_VIRT_H
#define LIBSIGMA_VIRT_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#elif defined(__STDC__)
#include <stdint.h>
#else 
#error "Compiling libsigma/virt.h on unknown language"
#endif

#define VCTL_CREATE_VCPU 0
#define VCTL_RUN_VCPU 1
#define VCTL_CREATE_VSPACE 2
#define VCTL_MAP_VSPACE 3

uint64_t vctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

#if defined(__cplusplus)
}
#endif

#endif