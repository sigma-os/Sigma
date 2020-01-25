#include <libsigma/virt.h>
#include <libsigma/syscall.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t vctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4){
    return libsigma_syscall5(SIGMA_SYSCALL_VCTL, cmd, arg1, arg2, arg3, arg4);
}

#ifdef __cplusplus
}
#endif