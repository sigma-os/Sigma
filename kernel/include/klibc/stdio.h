#ifndef SIGMA_KLIBC_STDIO
#define SIGMA_KLIBC_STDIO

#include <Sigma/arch/x86_64/vga.h>
#include <Sigma/arch/x86_64/serial.h>

#define EOF (-1)

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* format, ...);
int putchar(int c);
int puts(const char* s);


int debug_printf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif