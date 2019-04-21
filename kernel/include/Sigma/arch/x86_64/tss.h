#ifndef SIGMA_KERNEL_TSS
#define SIGMA_KERNEL_TSS

#include <Sigma/common.h>

namespace x86_64::tss
{
    constexpr uint8_t ist_n_entries = 7;

    C_LINKAGE uint64_t ist_stack1;
    C_LINKAGE uint64_t ist_stack2;
    C_LINKAGE uint64_t ist_stack3;
    C_LINKAGE uint64_t ist_stack4;
    C_LINKAGE uint64_t ist_stack5;
    C_LINKAGE uint64_t ist_stack6;
    C_LINKAGE uint64_t ist_stack7;

    struct table
    {
    public:
        table(){
            this->ist_stack1 = (uint64_t)&ist_stack1;
            this->ist_stack2 = (uint64_t)&ist_stack2;
            this->ist_stack3 = (uint64_t)&ist_stack3;
            this->ist_stack4 = (uint64_t)&ist_stack4;
            this->ist_stack5 = (uint64_t)&ist_stack5;
            this->ist_stack6 = (uint64_t)&ist_stack6;
            this->ist_stack7 = (uint64_t)&ist_stack7;

            this->reserved = 0;
            this->reserved_1 = 0;
            this->reserved_2 = 0;
            this->reserved_3 = 0;
            this->reserved_4 = 0;

            this->rsp0 = 0;
            this->rsp1 = 0;
            this->rsp2 = 0;

            this->io_bitmap_offset = 0;
        }


        uint32_t reserved;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;


        uint32_t reserved_1;
        uint32_t reserved_2;

        uint64_t ist_stack1;
        uint64_t ist_stack2;
        uint64_t ist_stack3;
        uint64_t ist_stack4;
        uint64_t ist_stack5;
        uint64_t ist_stack6;
        uint64_t ist_stack7;

        uint64_t reserved_3;
        uint16_t reserved_4;
        uint16_t io_bitmap_offset;

        void load(uint16_t gdt_offset){
            asm("mov %0, %%ax; ltr %%ax" : : "g"(gdt_offset) : "ax");
        }
    } __attribute__((packed));
} // x86_64::tss



#endif