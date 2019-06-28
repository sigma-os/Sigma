#ifndef SIGMA_KERNEL_MM_PMM
#define SIGMA_KERNEL_MM_PMM

#include <Sigma/common.h>
#include <Sigma/boot_protocol.h>
#include <Sigma/arch/x86_64/misc/spinlock.h>
#include <loader/3rdparty/multiboot.h>
#include <klibc/string.h>
#include <klibc/stdio.h>

// PMM should be fully global between ALL CPU's and threads and everything
namespace mm::pmm
{
    constexpr uint64_t block_size = 0x1000;

    void init(boot::boot_protocol* boot_protocol);
    void print_stack();

    void* alloc_block();
    void free_block(void* block);
} // mm::pmm






#endif