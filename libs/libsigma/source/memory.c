#include <libsigma/memory.h>
#include <libsigma/syscall.h>

uint64_t libsigma_valloc(uint64_t type, uint64_t base, uint64_t n_pages){
    return libsigma_syscall3(SIGMA_SYSCALL_VALLOC, type, base, n_pages);
}