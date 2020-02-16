#ifndef LIBSIGMA_SYSCALLS_H
#define LIBSIGMA_SYSCALLS_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stdbool.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stdbool.h>
#else 
#error "Compiling libsigma/syscall.h on unknown language"
#endif

enum {
    sigmaSyscallEarlyKlog = 0,

    sigmaSyscallSetFsBase,
    sigmaSyscallKill,
    sigmaSyscallFork,
    sigmaSyscallGetCurrentTid,
    sigmaSyscallBlockThread,

    sigmaSyscallVmMap,
    sigmaSyscallGetPhysRegion,

    sigmaSyscallReadInitrd,
    sigmaSyscallInitrdSize,

    sigmaSyscallIpcSend,
    sigmaSyscallIpcReceive,
    sigmaSyscallIpcGetSize,

    sigmaSyscallDevCtl,
    sigmaSyscallVCtl,
};

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