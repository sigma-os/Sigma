#include <libsigma/thread.h>
#include <libsigma/syscall.h>
#include "common.h"

int libsigma_set_fsbase(uint64_t fs){
    return libsigma_syscall1(SIGMA_SYSCALL_SET_FSBASE, fs);
}

NORETURN_ATTRIBUTE
void libsigma_kill(void){
    libsigma_syscall0(SIGMA_SYSCALL_KILL);
    while(1); // Why are we still here
              // Just to suffer
}

uint64_t libsigma_get_um_tid(void){
    return libsigma_syscall0(SIGMA_SYSCALL_GET_UM_TID);
}