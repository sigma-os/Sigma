#ifndef LIBSIGMA_IPC_H
#define LIBSIGMA_IPC_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stddef.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#else 
#error "Compiling libsigma/ipc.h on unknown language"
#endif


typedef struct libsigma_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;
    // TODO, anything more?
    uint8_t command_data[];
} libsigma_message_t;

bool libsigma_ipc_check_message(libsigma_message_t* msg, size_t size);
void libsigma_ipc_set_message_checksum(libsigma_message_t* msg, size_t size);

// TODO: tid_t
int libsigma_ipc_send(uint64_t dest, size_t buf_size, uint8_t* buffer);
int libsigma_ipc_receive(uint64_t* origin, size_t* buf_size, uint8_t* buffer);

size_t libsigma_ipc_get_msg_size();

enum {
    RET,
    OPEN,
    WRITE,
    DUP2
};

struct libsigma_ret_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int ret;
};


struct libsigma_open_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;
    
    int flags;
    size_t path_len;
    char path[];
};

struct libsigma_write_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int fd;
    size_t count;
    char buf[];
};

struct libsigma_dup2_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int oldfd;
    int newfd;
};

#if defined(__cplusplus)
}
#endif

#endif