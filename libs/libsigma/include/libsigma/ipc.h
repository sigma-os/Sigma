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

#include <libsigma/thread.h>

typedef struct libsigma_message {
    uint8_t byte;
    uint8_t data[];
} libsigma_message_t;

int libsigma_ipc_send(tid_t dest, libsigma_message_t* msg, size_t msg_size);
int libsigma_ipc_receive(tid_t* origin, libsigma_message_t* buffer, size_t* buf_size);

size_t libsigma_ipc_get_msg_size();

#if defined(__cplusplus)
}
#endif

#endif