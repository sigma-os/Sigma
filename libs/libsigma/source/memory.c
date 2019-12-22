#include <libsigma/memory.h>
#include <libsigma/syscall.h>

#ifdef __cplusplus
extern "C" {
#endif

void* libsigma_vm_map(size_t size, void *virt_addr, void* phys_addr, int prot, int flags){
    return (void*)libsigma_syscall5(SIGMA_SYSCALL_VM_MAP, (uint64_t)virt_addr, (uint64_t)phys_addr, size, prot, flags);
}

int libsigma_get_phys_region(size_t size, int prot, int flags, libsigma_phys_region_t* region){
    return libsigma_syscall4(SIGMA_SYSCALL_GET_PHYS_REGION, size, prot, flags, (uint64_t)region);
}

#ifdef __cplusplus
}
#endif