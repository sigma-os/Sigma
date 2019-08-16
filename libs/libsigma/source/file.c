#include <libsigma/file.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int libsigma_read_initrd_file(const char* filename, uint8_t* buffer, uint64_t offset, uint64_t length){
    return libsigma_syscall4(SIGMA_SYSCALL_READ_INITRD, (uint64_t)filename, (uint64_t)buffer, offset, length);
}

#ifdef __cplusplus
}
#endif