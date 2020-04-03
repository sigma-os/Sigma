#ifndef SIGMA_KERNEL_TSS
#define SIGMA_KERNEL_TSS

#include <Sigma/common.h>
#include <Sigma/mm/pmm.h>

namespace x86_64::tss
{
    constexpr uint8_t ist_n_entries = 7;


    struct PACKED_ATTRIBUTE table
    {
    public:
        table(){
            //TODO: only allocate needed stacks
            this->ist_stack1 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack2 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack3 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack4 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack5 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack6 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
            this->ist_stack7 = (reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);

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

        ~table(){
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack1 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack2 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack3 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack4 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack5 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack6 - KERNEL_VBASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack7 - KERNEL_VBASE));
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
            asm volatile("ltr %0" : : "r"(gdt_offset) : "memory");
        }
    };
} // x86_64::tss



#endif