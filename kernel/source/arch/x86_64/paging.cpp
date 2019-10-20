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

static inline void set_flags(uint64_t& entry, uint64_t flags){
    entry |= (flags & 0xFFF0000000000FFF);
}

static inline uint64_t get_flags(uint64_t entry){
    return (entry & 0xFFF0000000000FFF);
}

x86_64::paging::pml4* x86_64::paging::get_current_info(){
        uint64_t pointer = 0;
        asm("mov %%cr3, %0" : "=r"(pointer));
        return reinterpret_cast<x86_64::paging::pml4*>(pointer);
}

void x86_64::paging::set_current_info(x86_64::paging::pml4* info){
        uint64_t pointer = reinterpret_cast<uint64_t>(info);

        asm volatile("mov %0, %%cr3" : : "r"(pointer) : "memory"); // This ASM block has very imporant side effects
                                                                   // Namely the full trashing of the TLB and setting
                                                                   // of new page info
}

void x86_64::paging::paging::init(){
    if(this->paging_info != nullptr) this->deinit();

    this->paging_info = reinterpret_cast<x86_64::paging::pml4*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset_aligned_4k(reinterpret_cast<void*>(this->paging_info), 0);
}

void x86_64::paging::paging::invalidate_addr(uint64_t addr)
{
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory"); // This ASM block has the important side effects of
                                                         // Trashing a part of the TLB, thus it is set to be
                                                         // volatile and a memory border so it is exactly where
                                                         // we need it
}

static void clean_pd(x86_64::paging::pd* pd){
    for(uint64_t pd_loop_index = 0; pd_loop_index < x86_64::paging::paging_structures_n_entries; pd_loop_index++){
        uint64_t pt_entry = pd->entries[pd_loop_index];

        if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_present)){
            if(!(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_huge))){ // Not a huge page
                uint64_t pt_addr = (pt_entry & 0x000FFFFFFFFFF000);
                mm::pmm::free_block(reinterpret_cast<void*>(pt_addr));
            }
        }
    }
}

static void clean_pdpt(x86_64::paging::pdpt* pdpt){
    for(uint64_t pdpt_loop_index = 0; pdpt_loop_index < x86_64::paging::paging_structures_n_entries; pdpt_loop_index++){
        uint64_t pd_entry = pdpt->entries[pdpt_loop_index];

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
    if(this->paging_info == nullptr) return;


    for(uint64_t pml4_loop_index = 0; pml4_loop_index < x86_64::paging::paging_structures_n_entries; pml4_loop_index++){
        uint64_t pdpt_entry = this->paging_info->entries[pml4_loop_index];

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
        if(flags & map_page_flags_no_execute) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_no_execute);
        if(flags & map_page_flags_writable) bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_writeable);
    } else {
        return true; // Not present so no reason to map
    }

    uint64_t pml4_entry = this->paging_info->entries[pml4_index_number];

    if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present)){
        // PML4 entry not present create one

        uint64_t new_pml4_entry = entry_flags;

        uint64_t pdpt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());

        memset_aligned_4k(reinterpret_cast<void*>(pdpt + KERNEL_VBASE), 0);

        set_frame(new_pml4_entry, pdpt);
        this->paging_info->entries[pml4_index_number] = new_pml4_entry;
    }
    pml4_entry = this->paging_info->entries[pml4_index_number];
    auto pdpt_addr = (get_frame(pml4_entry) + KERNEL_VBASE);

    uint64_t pdpt_entry = reinterpret_cast<x86_64::paging::pdpt*>(pdpt_addr)->entries[pdpt_index_number];
    if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
        // PDPT entry not present create one

        uint64_t new_pdpt_entry = entry_flags;

        uint64_t pd = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());


        memset_aligned_4k(reinterpret_cast<void*>(pd + KERNEL_VBASE), 0);

        set_frame(new_pdpt_entry, pd);
        reinterpret_cast<x86_64::paging::pdpt*>(pdpt_addr)->entries[pdpt_index_number] = new_pdpt_entry;
    }
    pdpt_entry = reinterpret_cast<x86_64::paging::pdpt*>(pdpt_addr)->entries[pdpt_index_number];
    auto pd_addr = (get_frame(pdpt_entry) + KERNEL_VBASE);

    uint64_t pd_entry = reinterpret_cast<x86_64::paging::pd*>(pd_addr)->entries[pd_index_number];
    if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){
        // PD entry not present create one

        uint64_t new_pd_entry = entry_flags;
        uint64_t pt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        
        memset_aligned_4k(reinterpret_cast<void*>(pt + KERNEL_VBASE), 0);

        set_frame(new_pd_entry, pt);
        reinterpret_cast<x86_64::paging::pd*>(pd_addr)->entries[pd_index_number] = new_pd_entry;
    }
    pd_entry = reinterpret_cast<x86_64::paging::pd*>(pd_addr)->entries[pd_index_number];
    auto pt_addr = (get_frame(pd_entry) + KERNEL_VBASE);

    uint64_t pt_entry = entry_flags;
    if(flags & map_page_flags_global) bitops<uint64_t>::bit_set(pt_entry, x86_64::paging::page_entry_global);

    set_frame(pt_entry, phys);

    (reinterpret_cast<x86_64::paging::pt*>(pt_addr)->entries[pt_index_number]) = pt_entry;

    this->invalidate_addr(virt);

    return true;
}

void x86_64::paging::paging::set_paging_info(){
    uint64_t phys = (reinterpret_cast<uint64_t>(this->paging_info) - KERNEL_VBASE);

    asm volatile ("mov %0, %%cr3" : : "r"(phys) : "memory"); // This ASM block has very imporant side effects
                                                             // Namely the full trashing of the TLB and setting
                                                             // of new page info
}   



uint64_t x86_64::paging::paging::get_page_size(){
    return mm::pmm::block_size;
}

uint64_t x86_64::paging::paging::get_paging_info(){
    return reinterpret_cast<uint64_t>(this->paging_info);
}

static x86_64::paging::pt* clone_pt(x86_64::paging::pt* pt){
    x86_64::paging::pt* new_info_pt = reinterpret_cast<x86_64::paging::pt*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset_aligned_4k(static_cast<void*>(new_info_pt), 0);

    for(uint64_t i = 0; i < x86_64::paging::paging_structures_n_entries; i++){
        new_info_pt->entries[i] = pt->entries[i]; // Just copy it over
    }

    return reinterpret_cast<x86_64::paging::pt*>(reinterpret_cast<uint64_t>(new_info_pt) - KERNEL_VBASE);
}

static x86_64::paging::pd* clone_pd(x86_64::paging::pd* pd){
    x86_64::paging::pd* new_info_pd = reinterpret_cast<x86_64::paging::pd*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset_aligned_4k(static_cast<void*>(new_info_pd), 0);

    for(uint64_t i = 0; i < x86_64::paging::paging_structures_n_entries; i++){
        uint64_t old_pd_entry = pd->entries[i];

        uint64_t pd_entry_flags = get_flags(old_pd_entry);

        if(bitops<uint64_t>::bit_test(pd_entry_flags, x86_64::paging::page_entry_present)){
            if(!bitops<uint64_t>::bit_test(pd_entry_flags, x86_64::paging::page_entry_huge)){
                // Present and not huge, copy PT
                uint64_t pd = reinterpret_cast<uint64_t>(clone_pt(reinterpret_cast<x86_64::paging::pt*>(get_frame(old_pd_entry) + KERNEL_VBASE)));

                uint64_t new_entry = 0;
                set_frame(new_entry, pd);
                set_flags(new_entry, get_flags(old_pd_entry));

                new_info_pd->entries[i] = new_entry;
            } else {
                new_info_pd->entries[i] = old_pd_entry; // Just copy the (unused) / huge flags and page over
            }
        }

    }

    return reinterpret_cast<x86_64::paging::pd*>(reinterpret_cast<uint64_t>(new_info_pd) - KERNEL_VBASE);
}

static x86_64::paging::pdpt* clone_pdpt(x86_64::paging::pdpt* pdpt){
    x86_64::paging::pdpt* new_info_pdpt = reinterpret_cast<x86_64::paging::pdpt*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset_aligned_4k(static_cast<void*>(new_info_pdpt), 0);


    for(uint64_t i = 0; i < x86_64::paging::paging_structures_n_entries; i++){
        uint64_t old_pdpt_entry = pdpt->entries[i];

        uint64_t pdpt_entry_flags = get_flags(old_pdpt_entry);
        if(bitops<uint64_t>::bit_test(pdpt_entry_flags, x86_64::paging::page_entry_present)){
            if(!bitops<uint64_t>::bit_test(pdpt_entry_flags, x86_64::paging::page_entry_huge)){
                // Present and not huge, copy PD
                uint64_t pd = reinterpret_cast<uint64_t>(clone_pd(reinterpret_cast<x86_64::paging::pd*>(get_frame(old_pdpt_entry) + KERNEL_VBASE)));

                uint64_t new_entry = 0;
                set_frame(new_entry, pd);
                set_flags(new_entry, get_flags(old_pdpt_entry));

                new_info_pdpt->entries[i] = new_entry;
            } else {
                new_info_pdpt->entries[i] = old_pdpt_entry; // Just copy the (unused) / huge flags and page over
            }
        }
    }

    return reinterpret_cast<x86_64::paging::pdpt*>(reinterpret_cast<uint64_t>(new_info_pdpt) - KERNEL_VBASE);
}

void x86_64::paging::paging::clone_paging_info(IPaging& new_info){
    new_info.deinit();

    new_info.init();

    x86_64::paging::pml4* new_info_pml4 = reinterpret_cast<x86_64::paging::pml4*>(new_info.get_paging_info());

    for(uint64_t i = 0; i < x86_64::paging::paging_structures_n_entries / 2; i++){
        uint64_t old_pml4_entry = this->paging_info->entries[i];

        uint64_t pml4_entry_flags = get_flags(old_pml4_entry);
        if(bitops<uint64_t>::bit_test(pml4_entry_flags, x86_64::paging::page_entry_present)){
            if(!bitops<uint64_t>::bit_test(pml4_entry_flags, x86_64::paging::page_entry_huge)){
                // Present and not huge, copy PDPT
                uint64_t pdpt = reinterpret_cast<uint64_t>(clone_pdpt(reinterpret_cast<x86_64::paging::pdpt*>(get_frame(old_pml4_entry) + KERNEL_VBASE)));

                uint64_t new_entry = 0;
                set_frame(new_entry, pdpt);
                set_flags(new_entry, get_flags(old_pml4_entry));

                new_info_pml4->entries[i] = new_entry;
            } else {
                printf("[PAGING]: Illegal Huge Flag in PML4 entry\n");
                new_info_pml4->entries[i] = old_pml4_entry; // Just copy the huge flags and page over
            }
        } // Don't do anything with non-present entries, swapping is not implemented yet

    }

    for(uint64_t i = x86_64::paging::paging_structures_n_entries / 2; i < x86_64::paging::paging_structures_n_entries; i++){
        uint64_t old_pml4_entry = this->paging_info->entries[i];

        new_info_pml4->entries[i] = old_pml4_entry; // Just the page over
    }
}

uint64_t x86_64::paging::paging::get_phys(uint64_t virt){
    uint64_t pml4_index_number = pml4_index(virt);
    uint64_t pdpt_index_number = pdpt_index(virt);
    uint64_t pd_index_number = pd_index(virt);
    uint64_t pt_index_number = pt_index(virt);

    uint64_t pml4_entry = this->paging_info->entries[pml4_index_number];

    if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present)){
        return 0;
    }   

    x86_64::paging::pdpt* pdpt = reinterpret_cast<x86_64::paging::pdpt*>(this->paging_info->entries[pml4_index_number] + KERNEL_VBASE);
    uint64_t pdpt_entry = pdpt->entries[pdpt_index_number];
    if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
        return 0;
    }   

    x86_64::paging::pd* pd = reinterpret_cast<x86_64::paging::pd*>(pdpt->entries[pdpt_index_number] + KERNEL_VBASE);
    uint64_t pd_entry = pd->entries[pd_index_number];
    if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){
        return 0;
    }   
    x86_64::paging::pt* pt = reinterpret_cast<x86_64::paging::pt*>(pd->entries[pt_index_number] + KERNEL_VBASE);
    uint64_t pt_entry = pt->entries[pt_index_number];
    if(!bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_present)){
        return 0;
    }   

    return get_frame(pt_entry);
}

uint64_t x86_64::paging::paging::get_free_range(uint64_t search_base_hint, uint64_t search_end_hint, size_t size){
    for(uint64_t i = search_base_hint; i < search_end_hint; i += mm::pmm::block_size){
        uint64_t pml4_index_number = pml4_index(i);
        uint64_t pdpt_index_number = pdpt_index(i);
        uint64_t pd_index_number = pd_index(i);
        uint64_t pt_index_number = pt_index(i);

        uint64_t pml4_entry = this->paging_info->entries[pml4_index_number];
        if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present)){
            // 0x8000000000 is 512GiB thus PML4 size
            uint64_t pml4_base = i & ~(0x8000000000 - 1);
            if(size <= 0x8000000000){
                if(pml4_base == 0) continue;
                else return pml4_base;
            } else {
                uint64_t n_pml4s = misc::div_ceil(size, 0x8000000000);
                for(uint64_t j = 0; j < n_pml4s; j++){
                    uint64_t new_pml4_entry = this->paging_info->entries[pml4_index(pml4_base + (0x8000000000 * j))];
                    if(bitops<uint64_t>::bit_test(new_pml4_entry, x86_64::paging::page_entry_present)){
                       return 0; 
                    }
                }
                if(pml4_base == 0) continue;
                else return pml4_base;
            }
        }   

        x86_64::paging::pdpt* pdpt = reinterpret_cast<x86_64::paging::pdpt*>(this->paging_info->entries[pml4_index_number] + KERNEL_VBASE);
        uint64_t pdpt_entry = pdpt->entries[pdpt_index_number];
        if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
            // 0x40000000 is 1GiB thus PDPT size
            uint64_t pdpt_base = i & ~(0x40000000 - 1);
            if(size <= 0x40000000){
                if(pdpt_base == 0) continue;
                else return pdpt_base;
            } else {
                uint64_t n_pdpts = misc::div_ceil(size, 0x40000000);
                for(uint64_t j = 0; j < n_pdpts; j++){
                    uint64_t new_pdpt_entry = pdpt->entries[pdpt_index(pdpt_base + (0x40000000 * j))];
                    if(bitops<uint64_t>::bit_test(new_pdpt_entry, x86_64::paging::page_entry_present)){
                       return 0; 
                    }
                }
                if(pdpt_base == 0) continue;
                else return pdpt_base;
            }
        }   

        x86_64::paging::pd* pd = reinterpret_cast<x86_64::paging::pd*>(pdpt->entries[pdpt_index_number] + KERNEL_VBASE);
        uint64_t pd_entry = pd->entries[pd_index_number];
        if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){  
            // 0x200000 is 2MiB thus PD size
            uint64_t pd_base = i & ~(0x200000 - 1);
            if(size <= 0x200000){
                if(pd_base == 0) continue;
                else return pd_base;  
            } else {
                uint64_t n_pds = misc::div_ceil(size, 0x200000);
                for(uint64_t j = 0; j < n_pds; j++){
                    uint64_t new_pd_entry = pd->entries[pd_index(pd_base + (0x200000 * j))];
                    if(bitops<uint64_t>::bit_test(new_pd_entry, x86_64::paging::page_entry_present)){
                       return 0; 
                    }
                }
                if(pd_base == 0) continue;
                else return pd_base;
            }
        }   
        x86_64::paging::pt* pt = reinterpret_cast<x86_64::paging::pt*>(pd->entries[pt_index_number] + KERNEL_VBASE);
        uint64_t pt_entry = pt->entries[pt_index_number];
        if(!bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_present)){
            // 0x1000 is 4KiB thus PT size
            uint64_t pt_base = i & ~(0x1000 - 1);
            if(size <= 0x1000){
                if(pt_base == 0) continue;
                else return pt_base;  
            } else {
                uint64_t n_pts = misc::div_ceil(size, 0x1000);
                for(uint64_t j = 0; j < n_pts; j++){
                    uint64_t new_pt_entry = pd->entries[pt_index(pt_base + (0x1000 * j))];
                    if(bitops<uint64_t>::bit_test(new_pt_entry, x86_64::paging::page_entry_present)){
                       return 0; 
                    }
                }
                if(pt_base == 0) continue;
                else return pt_base;  
            }
        }
    }

    return 0;
}