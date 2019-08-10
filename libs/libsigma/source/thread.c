#include <libsigma/thread.h>
#include <libsigma/syscall.h>

int libsigma_set_fsbase(uint64_t fs){
    return libsigma_syscall1(SIGMA_SYSCALL_SET_FSBASE, fs);
}