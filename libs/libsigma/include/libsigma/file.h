#ifndef LIGSIGMA_FILE_H
#define LIGSIGMA_FILE_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stddef.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#else 
#error "Compiling libsigma/file.h on unknown language"
#endif

int libsigma_read_initrd_file(const char* filename, uint8_t* buffer, uint64_t offset, uint64_t length);
size_t libsigma_initrd_get_file_size(const char* filename);


#if defined(__cplusplus)
}
#endif

#endif