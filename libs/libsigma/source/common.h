#ifndef LIBSIGMA_COMMON_H
#define LIBSIGMA_COMMON_H

#if defined(__GNUC__) || defined(__clang__)
#define ALWAYSINLINE_ATTRIBUTE __attribute__((always_inline))
#define NORETURN_ATTRIBUTE __attribute__((__noreturn__))
#else
#error "Compiling libsigma on an unknown compiler"
#endif

#endif