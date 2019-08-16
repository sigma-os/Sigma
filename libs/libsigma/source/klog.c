#include <libsigma/klog.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int libsigma_klog(const char* str){
    return libsigma_syscall1(SIGMA_SYSCALL_EARLY_KLOG, (uint64_t)str);
}

#ifdef __cplusplus
}
#endif