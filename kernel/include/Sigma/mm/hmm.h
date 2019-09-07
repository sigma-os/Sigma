#ifndef SIGMA_KERNEL_MM_HEAP
#define SIGMA_KERNEL_MM_HEAP

#include <Sigma/common.h>
#include <Sigma/mm/alloc.h>

namespace mm::hmm
{
    void init();
    void* kmalloc(size_t size);
    void* kmalloc_a(size_t size, uint64_t align);
    void kfree(void* ptr);
    void* realloc(void* ptr, size_t size);
} // mm::hmm


#endif