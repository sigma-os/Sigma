#ifndef LIGSIGMA_THREAD_H
#define LIGSIGMA_THREAD_H

#if defined(__cplusplus)
#include <cstdint>
using std::uint64_t;
extern "C" {
#elif defined(__STDC__)
#include <stdint.h>
#else 
#error "Compiling libsigma/memory.h on unknown language"
#endif

#define LIBSIGMA_VALLOC_TYPE_SBRK_LIKE 0
#define LIBSIGMA_VALLOC_TYPE_FREE_BSAE 1

uint64_t libsigma_valloc(uint64_t type, uint64_t base, uint64_t n_pages);

#if defined(__cplusplus)
}
#endif

#endif