#include <libsigma/ipc.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

int libsigma_ipc_send(tid_t dest, libsigma_message_t* msg, size_t msg_size){
    return libsigma_syscall3(SIGMA_SYSCALL_IPC_SEND, dest, msg_size, (uint64_t)msg);
}

int libsigma_ipc_receive(tid_t* origin, libsigma_message_t* msg, size_t* msg_size){
    int ret = libsigma_syscall3(SIGMA_SYSCALL_IPC_RECEIVE, (uint64_t)msg, (uint64_t)msg_size, (uint64_t)origin);
    return ret;
}

size_t libsigma_ipc_get_msg_size(){
    return libsigma_syscall0(SIGMA_SYSCALL_IPC_GET_SIZE);
}

#ifdef __cplusplus
}
#endif