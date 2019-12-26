#include <libsigma/ipc.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

static bool check_message(libsigma_message_t* msg, size_t size){
    uint8_t check = 0;
    uint8_t* raw = (uint8_t*)msg;
    for(uint64_t i = 0; i < size; i++) check += raw[i];
    return (check == 0);
}

static void set_message_checksum(libsigma_message_t* msg, size_t size){
    msg->checksum = 0;

    uint8_t checksum = 0;
    uint8_t* raw = (uint8_t*)msg;
    for(uint64_t i = 0; i < size; i++) checksum += raw[i];

    msg->checksum = (UINT8_MAX + 1) - checksum;
}

int libsigma_ipc_send(tid_t dest, libsigma_message_t* msg, size_t msg_size){
    set_message_checksum(msg, msg_size);

    return libsigma_syscall3(SIGMA_SYSCALL_IPC_SEND, dest, msg_size, (uint64_t)msg);
}

int libsigma_ipc_receive(tid_t* origin, libsigma_message_t* msg, size_t* msg_size){
    int ret = libsigma_syscall3(SIGMA_SYSCALL_IPC_RECEIVE, (uint64_t)msg, (uint64_t)msg_size, (uint64_t)origin);

    if(!check_message(msg, *msg_size))
        return 1;
    
    return ret;
}

size_t libsigma_ipc_get_msg_size(){
    return libsigma_syscall0(SIGMA_SYSCALL_IPC_GET_SIZE);
}

#ifdef __cplusplus
}
#endif