#ifndef SIGMA_KERNEL_MM_PMM
#define SIGMA_KERNEL_MM_PMM

#include <Sigma/common.h>
#include <Sigma/multiboot.h>

#include <klibc/string.h>

// PMM should be fully global between ALL CPU's and threads and everything
namespace mm::pmm
{
    void init(multiboot& mb_info);

    void* alloc_block();
    void free_block(void* block);
} // mm::pmm






#endif