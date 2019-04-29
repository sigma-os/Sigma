#ifndef SIGMA_KERNEL_MM_HEAP
#define SIGMA_KERNEL_MM_HEAP

#include <Sigma/common.h>
#include <Sigma/mm/slab.h>

// Just using the SLAB allocator now, implement another type for array allocations
namespace mm::hmm
{
    void init(IPaging& vmm);
    void* kmalloc(size_t size);
    void kfree(void* ptr);
} // mm::hmm


#endif