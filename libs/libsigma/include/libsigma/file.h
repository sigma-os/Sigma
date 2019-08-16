#ifndef LIGSIGMA_FILE_H
#define LIGSIGMA_FILE_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#elif defined(__STDC__)
#include <stdint.h>
#else 
#error "Compiling libsigma/file.h on unknown language"
#endif

int libsigma_read_initrd_file(const char* filename, uint8_t* buffer, uint64_t offset, uint64_t length);

#if defined(__cplusplus)
}
#endif

#endif