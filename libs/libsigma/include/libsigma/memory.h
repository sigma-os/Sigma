#ifndef LIGSIGMA_THREAD_H
#define LIGSIGMA_THREAD_H

#if defined(__cplusplus)
#include <cstdint>
#include <cstddef>
using std::uint64_t;
using std::size_t;
extern "C" {
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#else 
#error "Compiling libsigma/memory.h on unknown language"
#endif

#define LIBSIGMA_VALLOC_TYPE_SBRK_LIKE 0
#define LIBSIGMA_VALLOC_TYPE_FREE_BSAE 1

uint64_t libsigma_valloc(uint64_t type, uint64_t base, uint64_t n_pages);
int libsigma_vm_map(size_t size, void *addr, int prot, int flags);

#if defined(__cplusplus)
}
#endif

#endif