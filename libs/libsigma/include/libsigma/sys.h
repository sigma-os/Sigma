#ifndef LIBSIGMA_SYS_H
#define LIBSIGMA_SYS_H

#if defined(__cplusplus)
extern "C" {
#include <stdint.h>
#include <stddef.h>
#elif defined(__STDC__)
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#else 
#error "Compiling libsigma/syscall.h on unknown language"
#endif

typedef uint64_t handle_t;
typedef uint64_t tid_t;

enum {
    devCtlNop = 0,
    devCtlClaim = 1,
    devCtlFindPci = 2,
    devCtlFindPciClass = 3,
    devCtlGetResourceRegion = 4,
    devCtlEnableIrq = 5,
    devCtlWaitOnIrq = 6,
    devCtlReadPci = 7,
    devCtlWritePci = 8,
};

enum {
    resourceRegionOriginPciBar = 0,

    resourceRegionTypeMmio = 0,
    resourceRegionTypeIo = 1,
    resourceRegionTypeInvalid = 0xFF
}; 

typedef struct libsigma_resource_region { 
    uint8_t type;
    uint64_t origin;
    uint64_t base;
    uint64_t len;
} libsigma_resource_region_t;

uint64_t devctl(uint64_t command, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

int libsigma_read_initrd_file(const char* filename, uint8_t* buffer, uint64_t offset, uint64_t length);
size_t libsigma_initrd_get_file_size(const char* filename);

int libsigma_set_fsbase(uint64_t fs);
void libsigma_kill(void);
uint64_t libsigma_fork(void);

tid_t libsigma_get_current_tid(void);

enum libsigma_block_reasons{SIGMA_BLOCK_FOREVER = 0, SIGMA_BLOCK_WAITING_FOR_IPC};
int libsigma_block_thread(enum libsigma_block_reasons reason);

typedef struct libsigma_message {
    uint8_t byte;
    uint8_t data[];
} libsigma_message_t;

int libsigma_ipc_send(tid_t dest, libsigma_message_t* msg, size_t msg_size);
int libsigma_ipc_receive(tid_t* origin, libsigma_message_t* buffer, size_t* buf_size);

size_t libsigma_ipc_get_msg_size();

void* libsigma_vm_map(size_t size, void *virt_addr, void* phys_addr, int prot, int flags);

typedef struct {
    uint64_t physical_addr;
    uint64_t virtual_addr;
    size_t size;
} libsigma_phys_region_t;

int libsigma_get_phys_region(size_t size, int prot, int flags, libsigma_phys_region_t* region);

int libsigma_klog(const char* str);

#ifdef __cplusplus
}
#endif

#endif