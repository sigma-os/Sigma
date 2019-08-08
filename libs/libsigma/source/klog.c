#include <libsigma/klog.h>
#include <libsigma/syscall.h>

int libsima_klog(const char* str){
    return libsigma_syscall1(SIGMA_SYSCALL_EARLY_KLOG, (uint64_t)str);
}