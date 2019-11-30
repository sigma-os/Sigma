#include <libsigma/memory.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t libsigma_valloc(uint64_t type, uint64_t base, uint64_t n_pages){
    return libsigma_syscall3(SIGMA_SYSCALL_VALLOC, type, base, n_pages);
}

void* libsigma_vm_map(size_t size, void *virt_addr, void* phys_addr, int prot, int flags){
    return (void*)libsigma_syscall5(SIGMA_SYSCALL_VM_MAP, (uint64_t)virt_addr, (uint64_t)phys_addr, size, prot, flags);
}

#ifdef __cplusplus
}
#endif