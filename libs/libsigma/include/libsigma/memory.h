#ifndef LIBSIGMA_MEMORY_H
#define LIBSIGMA_MEMORY_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stddef.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#else 
#error "Compiling libsigma/memory.h on unknown language"
#endif

#define LIBSIGMA_VALLOC_TYPE_SBRK_LIKE 0
#define LIBSIGMA_VALLOC_TYPE_FREE_BASE 1

uint64_t libsigma_valloc(uint64_t type, uint64_t base, uint64_t n_pages);
void* libsigma_vm_map(size_t size, void *virt_addr, void* phys_addr, int prot, int flags);

typedef struct {
    uint64_t physical_addr;
    uint64_t virtual_addr;
    size_t size;
} libsigma_phys_region_t;

int libsigma_get_phys_region(size_t size, int prot, int flags, libsigma_phys_region_t* region);

#if defined(__cplusplus)
}
#endif

#endif