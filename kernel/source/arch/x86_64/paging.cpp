#include <Sigma/arch/x86_64/paging.h>
#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/smp/cpu.h>
#include <Sigma/proc/process.h>

constexpr uint64_t pml4_index(uint64_t address){
    return (address >> 39) & 0x1FF;
}

constexpr uint64_t pdpt_index(uint64_t address){
   return (address >> 30) & 0x1FF;
}

constexpr uint64_t pd_index(uint64_t address){
    return (address >> 21) & 0x1FF;
}

constexpr uint64_t pt_index(uint64_t address){
    return (address >> 12) & 0x1FF;
}

constexpr uint64_t indicies_to_addr(uint64_t pml4_index, uint64_t pdpt_index, uint64_t pd_index, uint64_t index) {
    uint64_t virt_addr = 0;

    virt_addr |= (pml4_index << 39);
    virt_addr |= (pdpt_index << 30);
    virt_addr |= (pd_index << 21);
    virt_addr |= (index << 12);

    return virt_addr;
}

static inline void set_frame(uint64_t& entry, uint64_t phys){
    entry |= (phys & 0x000FFFFFFFFFF000);
}

ALWAYSINLINE_ATTRIBUTE
static inline uint64_t get_frame(uint64_t entry){
    return entry & 0x000FFFFFFFFFF000;
}

static inline void set_flags(uint64_t& entry, uint64_t flags){
    entry |= (flags & 0xFFF0000000000FFF);
}

ALWAYSINLINE_ATTRIBUTE
static inline uint64_t get_flags(uint64_t entry){
    return (entry & 0xFFF0000000000FFF);
}

ALWAYSINLINE_ATTRIBUTE
static inline uint64_t get_pat_flags(map_page_chache_types types){
    uint64_t ret = 0;
    switch (types)
    {
    case map_page_chache_types::normal: // Default is that so just don't
        FALLTHROUGH_ATTRIBUTE;
    case map_page_chache_types::write_back:
        break;
    case map_page_chache_types::write_combining:
        bitops<uint64_t>::bit_set(ret, x86_64::paging::page_entry_write_through);
        break;
    case map_page_chache_types::write_through:
        bitops<uint64_t>::bit_set(ret, x86_64::paging::page_entry_cache_disable);
        break;
    case map_page_chache_types::uncacheable:
        bitops<uint64_t>::bit_set(ret, x86_64::paging::page_entry_write_through);
        bitops<uint64_t>::bit_set(ret, x86_64::paging::page_entry_pat);
        break;
    }

    return ret;
}

x86_64::paging::pml4* x86_64::paging::get_current_info(){
        uint64_t pointer = 0;
        asm("mov %%cr3, %0" : "=r"(pointer));
        return reinterpret_cast<x86_64::paging::pml4*>(pointer);
}

void x86_64::paging::set_current_info(x86_64::paging::paging* info){
    auto* cpu = smp::cpu::entry::get_cpu();
    auto& cpu_context = cpu->pcid_context;

    if(cpu->features.pcid){ 

        uint16_t pcid = 0;
        for(uint16_t i = 0; i < n_pcids; i++){
            auto& pcid_context = cpu_context.contexts[i];
            
            auto* page_context = pcid_context.get_context();

            if(page_context && page_context == info){
                if(!pcid_context.is_active())
                    pcid_context.set_context();

                return;
            }

            // Take older PCID
            if(pcid_context.get_timestamp() < cpu_context.contexts[pcid].get_timestamp())
                pcid = i;
        }

        cpu_context.contexts[pcid].set_context(info);

    } else {
        // No PCID, so just use the first one
        cpu_context.contexts[0].set_context(info);
    }
}

void x86_64::paging::invalidate_addr(uint64_t addr)
{
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory"); // This ASM block has the important side effects of
                                                         // Trashing a part of the TLB, thus it is set to be
                                                         // volatile and a memory border so it is exactly where
                                                         // we need it
}

void invalidate_pcid(uint16_t pcid){
    if(smp::cpu::entry::get_cpu()->features.invpcid){
        struct {
		    uint64_t pcid;
		    const void *address;
	    } pcid_descriptor;

	    pcid_descriptor.pcid = pcid;
	    pcid_descriptor.address = nullptr;

	    uint64_t type = 1;
        asm("invpcid %1, %0" : : "r"(type), "m"(pcid_descriptor) : "memory");
    } else {
        uint64_t kernel_phys = reinterpret_cast<uint64_t>(mm::vmm::kernel_vmm::get_instance().get_paging_info()) - KERNEL_VBASE;
        x86_64::emulate_invpcid(kernel_phys, pcid);
    }
}

#pragma region paging::pcid_context
uint16_t x86_64::paging::pcid_context::get_pcid(){
    return pcid;
}

uint64_t x86_64::paging::pcid_context::get_timestamp(){
    return timestamp;
}

void x86_64::paging::pcid_context::set_context(){
    uint64_t table_phys = reinterpret_cast<uint64_t>(this->context->get_paging_info()) - KERNEL_VBASE;
    uint64_t cr3 = table_phys | this->pcid;
    if(smp::cpu::entry::get_cpu()->features.pcid)
        bitops<uint64_t>::bit_set(cr3, 63); // Do not invalidate the PCID

    asm volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");

    this->timestamp = smp::cpu::entry::get_cpu()->pcid_context.next_timestamp++;

    smp::cpu::entry::get_cpu()->pcid_context.active_context = this->pcid;
}

void x86_64::paging::pcid_context::set_context(x86_64::paging::paging* context){
    this->context = context;
    // TODO: TLB shootdown

    uint64_t table_phys = reinterpret_cast<uint64_t>(this->context->get_paging_info()) - KERNEL_VBASE;
    uint64_t cr3 = table_phys | this->pcid; // Invalidate PCID
    asm volatile ("mov %0, %%cr3" : : "r"(cr3) : "memory");

    this->timestamp = smp::cpu::entry::get_cpu()->pcid_context.next_timestamp++;

    smp::cpu::entry::get_cpu()->pcid_context.active_context = this->pcid;
}

bool x86_64::paging::pcid_context::is_active(){
    return this->pcid == smp::cpu::entry::get_cpu()->pcid_context.active_context;
}

x86_64::paging::paging* x86_64::paging::pcid_context::get_context(){
    return context;
}
#pragma endregion

#pragma region paging::paging

void x86_64::paging::paging::init(){
    if(this->paging_info != nullptr) this->deinit();

    this->paging_info = reinterpret_cast<x86_64::paging::pml4*>(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()) + KERNEL_VBASE);
    memset_aligned_4k(reinterpret_cast<void*>(this->paging_info), 0);
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

    for(uint64_t pml4_loop_index = 0; pml4_loop_index < x86_64::paging::paging_structures_n_entries / 2; pml4_loop_index++){
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

bool x86_64::paging::paging::map_page(uint64_t phys, uint64_t virt, uint64_t flags, map_page_chache_types cache){
    uint64_t pml4_index_number = pml4_index(virt);
    uint64_t pdpt_index_number = pdpt_index(virt);
    uint64_t pd_index_number = pd_index(virt);
    uint64_t pt_index_number = pt_index(virt);

    uint64_t entry_flags = 0;
    if(flags & map_page_flags_present) 
        bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_present);
    if(flags & map_page_flags_user) 
        bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_user);
    if(flags & map_page_flags_no_execute) 
        bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_no_execute);
    if(flags & map_page_flags_writable)
        bitops<uint64_t>::bit_set(entry_flags, x86_64::paging::page_entry_writeable);

    pml4* pml4_addr = this->paging_info;
    uint64_t pml4_entry = pml4_addr->entries[pml4_index_number];
    if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present)){
        // PML4 entry not present create one
        uint64_t new_pml4_entry = 0;
        uint64_t pdpt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset_aligned_4k(reinterpret_cast<void*>(pdpt + KERNEL_VBASE), 0);
        set_frame(new_pml4_entry, pdpt);
        set_flags(new_pml4_entry, entry_flags);

        pml4_addr->entries[pml4_index_number] = new_pml4_entry;
        pml4_entry = new_pml4_entry;
    }
    
    auto* pdpt = reinterpret_cast<x86_64::paging::pdpt*>(get_frame(pml4_entry) + KERNEL_VBASE);
    uint64_t pdpt_entry = pdpt->entries[pdpt_index_number];
    if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present)){
        // PDPT entry not present create one
        uint64_t new_pdpt_entry = 0;
        uint64_t pd = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset_aligned_4k(reinterpret_cast<void*>(pd + KERNEL_VBASE), 0);
        set_frame(new_pdpt_entry, pd);
        set_flags(new_pdpt_entry, entry_flags);

        pdpt->entries[pdpt_index_number] = new_pdpt_entry;
        pdpt_entry = new_pdpt_entry;
    }

    auto* pd = reinterpret_cast<x86_64::paging::pd*>(get_frame(pdpt_entry) + KERNEL_VBASE);
    uint64_t pd_entry = pd->entries[pd_index_number];
    if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present)){
        // PD entry not present create one
        uint64_t new_pd_entry = 0;
        uint64_t pt = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
        memset_aligned_4k(reinterpret_cast<void*>(pt + KERNEL_VBASE), 0);
        set_frame(new_pd_entry, pt);
        set_flags(new_pd_entry, entry_flags);

        pd->entries[pd_index_number] = new_pd_entry;
        pd_entry = new_pd_entry;
    }

    auto* pt = reinterpret_cast<x86_64::paging::pt*>(get_frame(pd_entry) + KERNEL_VBASE);

    uint64_t pt_entry = 0;
    set_frame(pt_entry, phys);
    set_flags(pt_entry, entry_flags);
    if(flags & map_page_flags_global) 
        bitops<uint64_t>::bit_set(pt_entry, x86_64::paging::page_entry_global);
    pt_entry |= get_pat_flags(cache);

    pt->entries[pt_index_number] = pt_entry;

    x86_64::paging::invalidate_addr(virt);
    return true;
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

void x86_64::paging::paging::clone_paging_info(x86_64::paging::paging& new_info){
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

void x86_64::paging::paging::set(){
    x86_64::paging::set_current_info(this);
}

uint64_t x86_64::paging::paging::get_paging_info(){
    return reinterpret_cast<uint64_t>(this->paging_info);
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

#pragma endregion

void x86_64::paging::paging::fork_address_space(proc::process::thread& new_thread){
    mm::vmm::kernel_vmm::get_instance().clone_paging_info(new_thread.vmm);

    for(uint64_t i = 0; i < (x86_64::paging::paging_structures_n_entries / 2); i++){
        uint64_t pml4_entry = this->paging_info->entries[i];

        if(!bitops<uint64_t>::bit_test(pml4_entry, x86_64::paging::page_entry_present))
            continue;

        x86_64::paging::pdpt* pdpt = reinterpret_cast<x86_64::paging::pdpt*>(get_frame(this->paging_info->entries[i]) + KERNEL_VBASE);
        for(uint64_t j = 0; j < x86_64::paging::paging_structures_n_entries; j++){
            uint64_t pdpt_entry = pdpt->entries[j];
            if(!bitops<uint64_t>::bit_test(pdpt_entry, x86_64::paging::page_entry_present))
                continue;

            x86_64::paging::pd* pd = reinterpret_cast<x86_64::paging::pd*>(get_frame(pdpt->entries[j]) + KERNEL_VBASE);
            for(uint64_t k = 0; k < x86_64::paging::paging_structures_n_entries; k++){
                uint64_t pd_entry = pd->entries[k];
                if(!bitops<uint64_t>::bit_test(pd_entry, x86_64::paging::page_entry_present))
                    continue;

                x86_64::paging::pt* pt = reinterpret_cast<x86_64::paging::pt*>(get_frame(pd->entries[k]) + KERNEL_VBASE);

                for(uint64_t l = 0; l < x86_64::paging::paging_structures_n_entries; l++){
                    uint64_t pt_entry = pt->entries[l];
                    if(!bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_present))
                        continue;

                    // Clone Page
                    uint64_t new_page_phys = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
                    uint64_t new_page_virt = indicies_to_addr(i, j, k, l);

                    uint64_t flags = map_page_flags_present;
                    if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_writeable))
                        flags |= map_page_flags_writable;
                    if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_user))
                        flags |= map_page_flags_user;
                    if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_no_execute))
                        flags |= map_page_flags_no_execute;
                    if(bitops<uint64_t>::bit_test(pt_entry, x86_64::paging::page_entry_global))
                        flags |= map_page_flags_global;

                    mm::vmm::kernel_vmm::get_instance().map_page(get_frame(pt_entry), (get_frame(pt_entry) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_global | map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);
                    mm::vmm::kernel_vmm::get_instance().map_page(new_page_phys, (new_page_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_global | map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);

                    memcpy_aligned_4k(reinterpret_cast<void*>(new_page_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), reinterpret_cast<void*>(get_frame(pt_entry) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE));

                    new_thread.vmm.map_page(new_page_phys, new_page_virt, flags);
                    new_thread.resources.frames.push_back(new_page_phys);
                }   
            }
        }
    }
}