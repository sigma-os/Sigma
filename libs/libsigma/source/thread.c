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

tid_t libsigma_get_um_tid(void){
    return libsigma_syscall0(SIGMA_SYSCALL_GET_UM_TID);
}

tid_t libsigma_get_current_tid(void){
    return libsigma_syscall0(SIGMA_SYSCALL_GET_CURRENT_TID);
}

int libsigma_block_thread(enum libsigma_block_reasons reason){
    return libsigma_syscall1(SIGMA_SYSCALL_BLOCK_THREAD, reason);
}

uint64_t libsigma_fork(void){
    return libsigma_syscall0(SIGMA_SYSCALL_FORK);
}