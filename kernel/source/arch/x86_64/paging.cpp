#include <Sigma/arch/x86_64/paging.h>

static inline uint64_t pml4_index(uint64_t address){
    //return (address >> 27) & 0x1FF;
    return (address >> 39) & 0x1FF;
}

static inline uint64_t pdpt_index(uint64_t address){
   /// return (address >> 18) & 0x1FF;
   return (address >> 30) & 0x1FF;
}

static inline uint64_t pd_index(uint64_t address){
    //return (address >> 9) & 0x1FF;
    return (address >> 21) & 0x1FF;
}

static inline uint64_t pt_index(uint64_t address){
    //return (address >> 0) & 0x1FF;
    return (address >> 12) & 0x1FF;
}

static inline void set_frame(uint64_t& entry, uint64_t phys){
    entry |= (phys & 0x000FFFFFFFFFF000);
}

static inline uint64_t get_frame(uint64_t entry){
    return entry & 0x000FFFFFFFFFF000;
}

void x86_64::paging::paging::init(){
    this->paging_info = reinterpret_cast<x86_64::paging::pml4*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset(reinterpret_cast<void*>(this->paging_info), 0, sizeof(x86_64::paging::pml4));
}

static void clean_pd(x86_64::paging::pd* pd){
    for(uint64_t pd_index = 0; pd_index < x86_64::paging::paging_structures_n_entries; pd_index++){
        uint64_t pt_entry = pd->entries[pd_index];

        if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_present)){
            if(!(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_huge))){ // Not a huge page
                uint64_t pt_addr = (pt_entry & 0x000FFFFFFFFFF000);
                mm::pmm::free_block(reinterpret_cast<void*>(pt_addr));
            }
        }
    }
}

static void clean_pdpt(x86_64::paging::pdpt* pdpt){
    for(uint64_t pdpt_index = 0; pdpt_index < x86_64::paging::paging_structures_n_entries; pdpt_index++){
        uint64_t pd_entry = pdpt->entries[pdpt_index];

        if(bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){
            if(!(bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_huge))){ // Not a huge page
                uint64_t pd_addr = ((pd_entry & 0x000FFFFFFFFFF000) + KERNEL_VBASE);
                clean_pd(reinterpret_cast<x86_64::paging::pd*>(pd_addr));

                mm::pmm::free_block(reinterpret_cast<void*>(pd_entry & 0x000FFFFFFFFFF000));
            }
        }
    }
}

void x86_64::paging::paging::deinit(){
    for(uint64_t pml4_index = 0; pml4_index < x86_64::paging::paging_structures_n_entries; pml4_index++){
        uint64_t pdpt_entry = this->paging_info->entries[pml4_index];

        if(bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
            if(!(bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_huge))){ // Not a huge page
                uint64_t pdpt_addr = ((pdpt_entry & 0x000FFFFFFFFFF000) + KERNEL_VBASE);
                clean_pdpt(reinterpret_cast<x86_64::paging::pdpt*>(pdpt_addr));

                mm::pmm::free_block(reinterpret_cast<void*>(pdpt_entry & 0x000FFFFFFFFFF000));
            }
        }
    }

    mm::pmm::free_block(reinterpret_cast<void*>(reinterpret_cast<uint64_t>(this->paging_info) - KERNEL_VBASE));
}   

bool x86_64::paging::paging::map_page(uint64_t phys, uint64_t virt, uint64_t flags){
    uint64_t pml4_index_number = pml4_index(virt);
    uint64_t pdpt_index_number = pdpt_index(virt);
    uint64_t pd_index_number = pd_index(virt);
    uint64_t pt_index_number = pt_index(virt);

    uint64_t entry_flags = 0;
    if(flags & map_page_flags_present){
        bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_present);
        if(flags & map_page_flags_cache_disable) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_cache_disable);
        if(flags & map_page_flags_user) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_user);
        if(flags & map_page_flags_no_execute) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_no_exectute);
        if(flags & map_page_flags_writable) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_writeable);
    } else {
        return true; // Not present so no reason to map
    }

    uint64_t& pml4_entry = this->paging_info->entries[pml4_index_number];

    if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present)){
        // PML4 entry not present create one

        uint64_t new_pml4_entry = entry_flags;

        uint64_t pdpt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset(reinterpret_cast<void*>(pdpt + KERNEL_VBASE), 0, sizeof(x86_64::paging::pdpt));

        set_frame(new_pml4_entry, pdpt);
        pml4_entry = new_pml4_entry;
    }
    auto pdpt_addr = (get_frame(pml4_entry) + KERNEL_VBASE);

    uint64_t& pdpt_entry = reinterpret_cast<x86_64::paging::pdpt*>(pdpt_addr)->entries[pdpt_index_number];
    if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
        // PDPT entry not present create one

        uint64_t new_pdpt_entry = entry_flags;

        uint64_t pd = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset(reinterpret_cast<void*>(pd + KERNEL_VBASE), 0, sizeof(x86_64::paging::pd));

        set_frame(new_pdpt_entry, pd);
        pdpt_entry = new_pdpt_entry;
    }
    auto pd_addr = (get_frame(pdpt_entry) + KERNEL_VBASE);

    uint64_t& pd_entry = reinterpret_cast<x86_64::paging::pd*>(pd_addr)->entries[pd_index_number];
    if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){
        // PD entry not present create one

        uint64_t new_pd_entry = entry_flags;
        uint64_t pt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset(reinterpret_cast<void*>(pt + KERNEL_VBASE), 0, sizeof(x86_64::paging::pt));

        set_frame(new_pd_entry, pt);
        pd_entry = new_pd_entry;
    }
    auto pt_addr = (get_frame(pd_entry) + KERNEL_VBASE);

    uint64_t& pt_entry = reinterpret_cast<x86_64::paging::pt*>(pt_addr)->entries[pt_index_number];

    pt_entry = entry_flags;
    set_frame(pt_entry, phys);

    return true;
}

void x86_64::paging::paging::set_paging_info(){
    uint64_t phys = (reinterpret_cast<uint64_t>(this->paging_info) - KERNEL_VBASE);

    asm volatile ("mov %0, %%cr3" : : "r"(phys));
}   

void x86_64::paging::paging::clone_paging_info(IPaging& new_info){
    (void)(new_info);
}

uint64_t x86_64::paging::paging::get_page_size(){
    return mm::pmm::block_size;
}