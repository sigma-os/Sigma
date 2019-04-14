#ifndef SIGMA_KLIBC_STRING
#define SIGMA_KLIBC_STRING

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t strlen(const char* s);

void* memset(void* s, int c, size_t n);


#ifdef __cplusplus
}
#endif


#endif