#ifndef SIGMA_KLIBC_STDLIB
#define SIGMA_KLIBC_STDLIB

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

char* itoa(int64_t value, char* str, int base);
__attribute__((__noreturn__)) void abort(void);
void htoa(int64_t n, char* str);

#ifdef __cplusplus
}
#endif


#endif