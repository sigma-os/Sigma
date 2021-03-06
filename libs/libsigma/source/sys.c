#include <libsigma/sys.h>
#include <libsigma/syscall.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t devctl(uint64_t command, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4){
    return libsigma_syscall5(sigmaSyscallDevCtl, command, arg1, arg2, arg3, arg4);
}

int libsigma_read_initrd_file(const char* filename, uint8_t* buffer, uint64_t offset, uint64_t length){
    return libsigma_syscall4(sigmaSyscallReadInitrd, (uint64_t)filename, (uint64_t)buffer, offset, length);
}

size_t libsigma_initrd_get_file_size(const char* filename){
    return libsigma_syscall1(sigmaSyscallInitrdSize, (uint64_t) filename);
}

int libsigma_ipc_send(handle_t ring, libsigma_message_t* msg, size_t msg_size){
    return libsigma_syscall3(sigmaSyscallIpcSend, ring, (uint64_t)msg, msg_size);
}

int libsigma_ipc_receive(handle_t ring, libsigma_message_t* msg){
    return libsigma_syscall2(sigmaSyscallIpcReceive, ring, (uint64_t)msg);
}

size_t libsigma_ipc_get_msg_size(handle_t ring){
    return libsigma_syscall1(sigmaSyscallIpcGetSize, ring);
}

int libsigma_klog(const char* str){
    return libsigma_syscall1(sigmaSyscallEarlyKlog, (uint64_t)str);
}

void* libsigma_vm_map(size_t size, void *virt_addr, void* phys_addr, int prot, int flags){
    return (void*)libsigma_syscall5(sigmaSyscallVmMap, (uint64_t)virt_addr, (uint64_t)phys_addr, size, prot, flags);
}

int libsigma_get_phys_region(size_t size, int prot, int flags, libsigma_phys_region_t* region){
    return libsigma_syscall4(sigmaSyscallGetPhysRegion, size, prot, flags, (uint64_t)region);
}

int libsigma_set_fsbase(uint64_t fs){
    return libsigma_syscall1(sigmaSyscallSetFsBase, fs);
}

NORETURN_ATTRIBUTE
void libsigma_kill(void){
    libsigma_syscall0(sigmaSyscallKill);
    while(1); // Why are we still here
              // Just to suffer
}

tid_t libsigma_get_current_tid(void){
    return libsigma_syscall0(sigmaSyscallGetCurrentTid);
}

int libsigma_block_thread(enum libsigma_block_reasons reason, handle_t handle){
    return libsigma_syscall2(sigmaSyscallBlockThread, reason, handle);
}

uint64_t libsigma_fork(void){
    return libsigma_syscall0(sigmaSyscallFork);
}

void libsigma_yield(void){
    libsigma_syscall0(sigmaSyscallYield);
}

#ifdef __cplusplus
}
#endif