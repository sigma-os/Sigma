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
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    uint8_t command_data[];
} libsigma_message_t;

int libsigma_ipc_send(tid_t dest, libsigma_message_t* msg, size_t msg_size);
int libsigma_ipc_receive(tid_t* origin, libsigma_message_t* buffer, size_t* buf_size);

size_t libsigma_ipc_get_msg_size();

enum {
    RET,
    OPEN,
    CLOSE,
    READ,
    WRITE,
    DUP2,
    SEEK,
    TELL
};

struct libsigma_ret_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    uint64_t ret;

    size_t size;
    char buf[];
    
    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};


struct libsigma_open_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;
    
    int flags;
    size_t path_len;
    char path[];

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_close_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;
    
    int fd;

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_read_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int fd;
    size_t count;

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_write_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int fd;
    size_t count;
    char buf[];

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_dup2_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int oldfd;
    int newfd;

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_seek_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int fd;
    uint64_t offset;
    int whence;

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

struct libsigma_tell_message {
    uint64_t command;
    uint8_t checksum;
    uint64_t msg_id;

    int fd;

    #if defined(__cplusplus)
    libsigma_message_t* msg(){
        return reinterpret_cast<libsigma_message_t*>(this);
    }

    uint8_t* data(){
        return reinterpret_cast<uint8_t*>(this);
    }
    #endif
};

#if defined(__cplusplus)
}
#endif

#endif