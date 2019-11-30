#ifndef LIBSIGMA_DEVICE_H
#define LIBSIGMA_DEVICE_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stddef.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#else 
#error "Compiling libsigma/device.h on unknown language"
#endif

#define DEVCTL_CMD_NOP 0
#define DEVCTL_CMD_CLAIM 1
#define DEVCTL_CMD_FIND_PCI 2
#define DEVCTL_CMD_FIND_PCI_CLASS 3
#define DEVCTL_CMD_GET_RESOURCE_REGION 4

#define RESOURCE_REGION_TYPE_MMIO 0
#define RESOURCE_REGION_TYPE_IO 1
#define RESOURCE_REGION_TYPE_INVALID 0xFF 

typedef struct libsigma_resource_region { 
    uint8_t type;
    uintptr_t base;
    size_t len;
} libsigma_resource_region_t;

uint64_t devctl(uint64_t command, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

#if defined(__cplusplus)
}
#endif

#endif