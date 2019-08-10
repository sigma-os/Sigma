#ifndef LIGSIGMA_THREAD_H
#define LIGSIGMA_THREAD_H

#if defined(__cplusplus)
#include <cstdint>
using std::uint64_t;
extern "C" {
#elif defined(__STDC__)
#include <stdint.h>
#else 
#error "Compiling libsigma/syscall.h on unknown language"
#endif

int libsigma_set_fsbase(uint64_t fs);

#if defined(__cplusplus)
}
#endif

#endif