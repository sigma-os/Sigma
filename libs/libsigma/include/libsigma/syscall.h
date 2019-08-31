#ifndef LIBSIGMA_SYSCALLS_H
#define LIBSIGMA_SYSCALLS_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#elif defined(__STDC__)
#include <stdint.h>
#else 
#error "Compiling libsigma/syscall.h on unknown language"
#endif

#define SIGMA_SYSCALL_EARLY_KLOG   0
#define SIGMA_SYSCALL_SET_FSBASE   1
#define SIGMA_SYSCALL_KILL         2
#define SIGMA_SYSCALL_VALLOC       3
#define SIGMA_SYSCALL_VM_MAP       4
#define SIGMA_SYSCALL_READ_INITRD  5
#define SIGMA_SYSCALL_INITRD_SIZE  6
#define SIGMA_SYSCALL_IPC_SEND     7
#define SIGMA_SYSCALL_IPC_RECEIVE  8
#define SIGMA_SYSCALL_IPC_GET_SIZE 9
#define SIGMA_SYSCALL_GET_UM_TID   10

uint64_t libsigma_syscall0(uint64_t number);
uint64_t libsigma_syscall1(uint64_t number, uint64_t arg1);
uint64_t libsigma_syscall2(uint64_t number, uint64_t arg1, uint64_t arg2);
uint64_t libsigma_syscall3(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3);
uint64_t libsigma_syscall4(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
uint64_t libsigma_syscall5(uint64_t number, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

#ifdef __cplusplus
}
#endif

#endif