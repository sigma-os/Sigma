#ifndef SIGMA_KERNEL_TSS
#define SIGMA_KERNEL_TSS

#include <Sigma/common.h>
#include <Sigma/mm/pmm.h>

namespace x86_64::tss
{
    constexpr uint8_t ist_n_entries = 7;

    struct PACKED_ATTRIBUTE table {
    public:
        void init(){
            //TODO: only allocate needed stacks
            this->ist_stack1 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack2 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack3 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack4 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack5 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack6 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            this->ist_stack7 = ((uintptr_t)mm::pmm::alloc_block() + mm::pmm::block_size + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

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

        void deinit(){
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack1 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack2 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack3 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack4 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack5 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack6 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
            mm::pmm::free_block(reinterpret_cast<void*>(this->ist_stack7 - mm::pmm::block_size - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));
        }

        void load(uint16_t gdt_offset){
            asm volatile("ltr %0" : : "r"(gdt_offset) : "memory");
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
    };
} // x86_64::tss



#endif