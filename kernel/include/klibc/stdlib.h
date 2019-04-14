#ifndef SIGMA_KLIBC_STDLIB
#define SIGMA_KLIBC_STDLIB

#ifdef __cplusplus
extern "C" {
#endif

char* itoa(int value, char* str, int base);
__attribute__((__noreturn__)) void abort(void);

#ifdef __cplusplus
}
#endif


#endif