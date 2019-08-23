#include <libsigma/ipc.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

bool libsigma_ipc_check_message(libsigma_message_t* msg, size_t size){
    uint8_t check = 0;
    uint8_t* raw = (uint8_t*)msg;
    for(uint64_t i = 0; i < size; i++) check += raw[i];
    return (check == 0);
}

void libsigma_ipc_set_message_checksum(libsigma_message_t* msg, size_t size){
    msg->checksum = 0;

    uint8_t checksum = 0;
    uint8_t* raw = (uint8_t*)msg;
    for(uint64_t i = 0; i < size; i++) checksum += raw[i];

    msg->checksum = 255 - checksum;
}

int libsigma_ipc_send(uint64_t dest, size_t buf_size, uint8_t* buffer){
    return libsigma_syscall3(SIGMA_SYSCALL_IPC_SEND, dest, buf_size, (uint64_t)buffer);
}

int libsigma_ipc_receive(uint64_t* origin, size_t* buf_size, uint8_t* buffer){
    return libsigma_syscall3(SIGMA_SYSCALL_IPC_RECEIVE, (uint64_t)buffer, (uint64_t)buf_size, (uint64_t)origin);
}

size_t libsigma_ipc_get_msg_size(){
    return libsigma_syscall0(SIGMA_SYSCALL_IPC_GET_SIZE);
}

#ifdef __cplusplus
}
#endif