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

x86_64::sl_paging::context::context(uint8_t level): level{level} {
    ASSERT(level <= 5 && level >= 3);
    const auto [phys, _] = create_table();

    this->phys_root = phys;
}

static void clean_tables(uint64_t phys, uint8_t level){
    auto virt = phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
    auto& pml = *(x86_64::sl_paging::sl_pml*)virt;
    if(level >= 3){
        for(size_t i = 0; i < 512; i++)
            if(pml[i].r)
                clean_tables(pml[i].addr << 12, level - 1);
    } else {
        for(size_t i = 0; i < 512; i++)
            if(pml[i].r)
                delete_table(pml[i].addr << 12);
    }
    delete_table(phys);
}

x86_64::sl_paging::context::~context(){
    clean_tables(this->phys_root, this->level);
}

void x86_64::sl_paging::context::map(uint64_t pa, uint64_t iova, uint64_t flags){
    uint64_t pml5_i = (iova >> 48) & 0x1FF;
    uint64_t pml4_i = (iova >> 39) & 0x1FF;
    uint64_t pml3_i = (iova >> 30) & 0x1FF;
    uint64_t pml2_i = (iova >> 21) & 0x1FF;
    uint64_t pml1_i = (iova >> 12) & 0x1FF;

    auto get_level_or_create = [](sl_pml* previous, uint64_t index) -> sl_pml* {
        auto& entry = (*previous)[index];
        if(!entry.r){
            const auto [phys, _] = create_table();
            entry.addr = (phys >> 12);
            entry.r = 1;
            entry.w = 1;
        }

        return (sl_pml*)((entry.addr << 12) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    };

    auto* current = (sl_pml*)(this->phys_root + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    if(level >= 5)
        current = get_level_or_create(current, pml5_i);
    if(level >= 4)
        current = get_level_or_create(current, pml4_i);
    current = get_level_or_create(current, pml3_i);
    current = get_level_or_create(current, pml2_i);
    
    auto& pml1 = *(sl_pml1*)current;
    auto& pml1_e = pml1[pml1_i];

    pml1_e.r = (flags & mapSlPageRead) ? 1 : 0;
    pml1_e.w = (flags & mapSlPageWrite) ? 1 : 0;
    pml1_e.x = (flags & mapSlPageExecute) ? 1 : 0;
    pml1_e.frame = (pa >> 12);
}

uint64_t x86_64::sl_paging::context::get_ptr(){
    return this->phys_root;
}