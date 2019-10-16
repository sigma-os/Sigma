#include <libsigma/klog.h>
#include <libsigma/thread.h>
#include <fcntl.h>
#include <libsigma/ipc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int testopen(){
    struct libsigma_open_message* openmsg = malloc(sizeof(struct libsigma_open_message) + 11);
    openmsg->command = OPEN;
    openmsg->msg_id = 0;
    openmsg->flags = O_WRONLY;
    char* path = "/dev/sysout";
    memcpy(openmsg->path, path, 11);
    openmsg->path_len = 11;
    libsigma_ipc_set_message_checksum((void*)openmsg, sizeof(struct libsigma_open_message) + 11);

    if(libsigma_ipc_send(libsigma_get_um_tid(), sizeof(struct libsigma_open_message) + 11, (uint8_t*)openmsg) == 1)
        libsigma_klog("Failed to send open");

    while(libsigma_ipc_get_msg_size() == 0); // No response yet

    size_t sz = libsigma_ipc_get_msg_size();
    struct libsigma_ret_message* openret = malloc(sz);
    uint64_t origin, useless;
    if(libsigma_ipc_receive(&origin, &useless, (void*)openret) == 1)
        libsigma_klog("Failed to receive open ret");

    if(!libsigma_ipc_check_message((void*)openret, sz)) libsigma_klog("Invalid message checksum");

    if(origin != libsigma_get_um_tid()) libsigma_klog("Didn't receive correct return");
    if(openret->msg_id != 0) libsigma_klog("Didn't receive correct msg id");

    if(openret->ret == -1) libsigma_klog("Failed to open file");
    return openret->ret;
}

void testdup2(int fd){
    struct libsigma_dup2_message* dupmsg = malloc(sizeof(struct libsigma_dup2_message));
    dupmsg->command = DUP2;
    dupmsg->msg_id = 0;
    dupmsg->oldfd = fd;
    dupmsg->newfd = STDOUT_FILENO;
    libsigma_ipc_set_message_checksum((void*)dupmsg, sizeof(struct libsigma_dup2_message));

    if(libsigma_ipc_send(libsigma_get_um_tid(), sizeof(struct libsigma_dup2_message), (uint8_t*)dupmsg) == 1)
        libsigma_klog("Failed to send DUP2");

    while(libsigma_ipc_get_msg_size() == 0); // No response yet

    size_t sz = libsigma_ipc_get_msg_size();
    struct libsigma_ret_message* dupret = malloc(sz);
    uint64_t origin, useless;
    if(libsigma_ipc_receive(&origin, &useless, (void*)dupret) == 1)
        libsigma_klog("Failed to receive dup2 ret");

    if(!libsigma_ipc_check_message((void*)dupret, sz)) libsigma_klog("Invalid message checksum");

    if(origin != libsigma_get_um_tid()) libsigma_klog("Didn't receive correct return");
    if(dupret->msg_id != 0) libsigma_klog("Didn't receive correct msg id");
}

void wrtest(){
    struct libsigma_write_message* writemsg = malloc(sizeof(struct libsigma_write_message) + 12);
    writemsg->command = WRITE;
    writemsg->msg_id = 0;
    writemsg->count = 12;
    writemsg->fd = STDOUT_FILENO;
    char* buf = "Hello World!";
    memcpy(writemsg->buf, buf, 12);
    libsigma_ipc_set_message_checksum((void*)writemsg, sizeof(struct libsigma_write_message) + 12);

    if(libsigma_ipc_send(libsigma_get_um_tid(), sizeof(struct libsigma_write_message) + 12, (uint8_t*)writemsg) == 1)
        libsigma_klog("Failed to send write");

    while(libsigma_ipc_get_msg_size() == 0); // No response yet

    size_t sz = libsigma_ipc_get_msg_size();
    struct libsigma_ret_message* writeret = malloc(sz);
    uint64_t origin, useless;
    if(libsigma_ipc_receive(&origin, &useless, (void*)writeret) == 1)
        libsigma_klog("Failed to receive write ret");

    if(!libsigma_ipc_check_message((void*)writeret, sz)) libsigma_klog("Invalid message checksum");

    if(origin != libsigma_get_um_tid()) libsigma_klog("Didn't receive correct return");
    if(writeret->msg_id != 0) libsigma_klog("Didn't receive correct msg id");
}

int main(int argc, char* argv[]){
    (void)(argc);
    (void)(argv);
    int fd = testopen();
    testdup2(fd);

    wrtest();

    return 0;
}