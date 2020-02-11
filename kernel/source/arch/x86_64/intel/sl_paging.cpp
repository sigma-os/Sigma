#include <Sigma/arch/x86_64/intel/sl_paging.hpp>
#include <Sigma/mm/vmm.h>

static std::pair<uint64_t, uint64_t> create_table(){
    uint64_t phys = (uint64_t)mm::pmm::alloc_block();
    uint64_t virt = phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
    mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);
    memset_aligned_4k((void*)virt, 0);

    return {phys, virt};
}

static void delete_table(uint64_t phys){
    mm::pmm::free_block((void*)phys);
}

x86_64::sl_paging::context::context(){
    const auto [pml4_phys, pml4_virt] = create_table();

    this->root = (sl_pml4*)pml4_virt;
    this->phys_root = pml4_phys;
}

x86_64::sl_paging::context::~context(){
    for(size_t pml4_i = 0; pml4_i < 512; pml4_i++){
        auto& pml4_e = this->root->entries[pml4_i];

        if(pml4_e.r){
            auto pml3_phys = (pml4_e.addr << 12);
            auto pml3_virt = pml3_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
            auto& pml3 = *(sl_pml3*)pml3_virt;

            for(size_t pml3_i = 0; pml3_i < 512; pml3_i++){
                auto& pml3_e = pml3[pml3_i];
                if(pml3_e.r){
                    auto pml2_phys = (pml3_e.addr << 12);
                    auto pml2_virt = pml2_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
                    auto& pml2 = *(sl_pml3*)pml2_virt;

                    for(size_t pml2_i = 0; pml2_i < 512; pml2_i++){
                        auto& pml2_e = pml2[pml2_i];
                        if(pml2_e.r){
                            delete_table(pml2_e.addr << 12);
                        }
                    }

                    delete_table(pml2_phys);
                }   
            }

            delete_table(pml3_phys);
        }
    }

    delete_table(this->phys_root);
}

void x86_64::sl_paging::context::map(uint64_t pa, uint64_t iova, uint64_t flags){
    uint64_t pml4_i = (iova >> 39) & 0x1FF;
    uint64_t pml3_i = (iova >> 30) & 0x1FF;
    uint64_t pml2_i = (iova >> 21) & 0x1FF;
    uint64_t pml1_i = (iova >> 12) & 0x1FF;

    auto& pml4_e = this->root->entries[pml4_i];
    if(!pml4_e.r){
        const auto [pml3_phys, _] = create_table();
        pml4_e.addr = (pml3_phys >> 12);
        pml4_e.r = 1;
        pml4_e.w = 1;
    }

    auto& pml3 = *(sl_pml3*)((pml4_e.addr << 12) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    auto& pml3_e = pml3[pml3_i];
    if(!pml3_e.r){
        const auto [pml2_phys, _] = create_table();
        pml3_e.addr = (pml2_phys >> 12);
        pml3_e.r = 1;
        pml3_e.w = 1;
    }

    auto& pml2 = *(sl_pml2*)((pml3_e.addr << 12) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    auto& pml2_e = pml2[pml2_i];
    if(!pml2_e.r){
        const auto [pml1_phys, _] = create_table();
        pml2_e.addr = (pml1_phys >> 12);
        pml2_e.r = 1;
        pml2_e.w = 1;
    }

    auto& pml1 = *(sl_pml1*)((pml2_e.addr << 12) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    auto& pml1_e = pml1[pml1_i];

    pml1_e.r = (flags & mapSlPageRead) ? 1 : 0;
    pml1_e.w = (flags & mapSlPageWrite) ? 1 : 0;
    pml1_e.x = (flags & mapSlPageExecute) ? 1 : 0;
    pml1_e.frame = (pa >> 12);
}

uint64_t x86_64::sl_paging::context::get_ptr(){
    return this->phys_root;
}