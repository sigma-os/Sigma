#ifndef SIGMA_KERNEL_TSS
#define SIGMA_KERNEL_TSS

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/ist.h>

namespace x86_64::tss
{
    struct table
    {
    public:
        uint32_t reserved;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;


        uint32_t reserved_1;
        uint32_t reserved_2;

        x86_64::ist::table ist;

        uint64_t reserved_3;
        uint16_t reserved_4;
        uint16_t io_bitmap_offset;

        void load(uint16_t gdt_offset){
            asm("mov %0, %%ax; ltr %%ax" : : "g"(gdt_offset) : "ax");
        }
    } __attribute__((packed));
} // x86_64::tss



#endif