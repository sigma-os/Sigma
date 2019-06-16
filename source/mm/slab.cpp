#include <Sigma/mm/slab.h>

uint64_t slab_addr = 0xffffffffD0000000;

mm::slab::slab* slab_list;

mm::slab::slab* slab_metadata;

static mm::slab::slab* alloc_slab_meta(){
    mm::slab::slab slab_meta;
    slab_meta.init(sizeof(mm::slab::slab));

    uint64_t slab_loc;

    bool success = slab_meta.alloc(sizeof(mm::slab::slab), slab_loc);

    if(!success) return nullptr;

    auto* new_slab_meta = reinterpret_cast<mm::slab::slab*>(slab_loc);
    *new_slab_meta = slab_meta;
    return new_slab_meta;
}


void mm::slab::slab::init(uint64_t size){
    this->slab_mutex = x86_64::spinlock::mutex();

    x86_64::spinlock::acquire(&this->slab_mutex);

    if(size == 0){
        printf("[SLAB]: Trying to create SLAB with allocation size 0, failing\n");
        x86_64::spinlock::release(&this->slab_mutex);
        return;
    }
    this->next_slab = nullptr;    
    this->size = size;

    mm::vmm::kernel_vmm::get_instance().map_page(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()), slab_addr, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute | map_page_flags_global);

    this->slab_start = slab_addr;
    slab_addr += mm::pmm::block_size;

    memset(reinterpret_cast<void*>(this->slab_start), 0, mm::pmm::block_size);

    uint64_t n_entries = (mm::pmm::block_size / size) - 1;

    this->free_list = reinterpret_cast<mm::slab::slab_entry*>(this->slab_start);//(mm::slab::slab_entry*)slab_start;

    slab_entry* current = this->free_list;
    for(uint64_t i = 0; i < n_entries; i++){
        current->next = reinterpret_cast<mm::slab::slab_entry*>(this->slab_start + (i * size));
        current = current->next;
    }
    x86_64::spinlock::release(&this->slab_mutex);
}

bool mm::slab::slab::alloc(size_t sz, uint64_t& loc){
    if(sz != this->size || this->free_list == nullptr) return false;

    x86_64::spinlock::acquire(&this->slab_mutex);

    loc = reinterpret_cast<uint64_t>(this->free_list);
    this->free_list = this->free_list->next;

    x86_64::spinlock::release(&this->slab_mutex);
    return true;
}

bool mm::slab::slab::free(uint64_t location){
    if((location < this->slab_start) || (location >= (this->slab_start + mm::pmm::block_size))) return false;
    
    x86_64::spinlock::acquire(&this->slab_mutex);

    auto* new_entry = reinterpret_cast<mm::slab::slab_entry*>(location);
    new_entry->next = this->free_list;
    this->free_list = new_entry;
    
    x86_64::spinlock::release(&this->slab_mutex);
    return true;
}

void mm::slab::slab_init(){
    slab_list = nullptr;

    slab_metadata = alloc_slab_meta();
}

void* mm::slab::slab_alloc(size_t size){
    uint64_t loc;

    for(mm::slab::slab* slab = slab_list; slab; slab = slab->next_slab){
        if(slab->alloc(size, loc)) return reinterpret_cast<void*>(loc);
    }

    uint64_t slab_loc;

    bool success = slab_metadata->alloc(sizeof(mm::slab::slab), slab_loc);
    if(!success){
        slab_metadata = alloc_slab_meta();

        slab_addr += mm::pmm::block_size;
        slab_metadata->alloc(sizeof(mm::slab::slab), slab_loc);
    }

    auto* new_slab = reinterpret_cast<mm::slab::slab*>(slab_loc);
    new_slab->init(size);
    new_slab->next_slab = slab_list;
    slab_list = new_slab;

    new_slab->alloc(size, loc);
    return reinterpret_cast<void*>(loc);
}

void mm::slab::slab_free(void* ptr){
    if(ptr == nullptr) return;

    auto loc = reinterpret_cast<uint64_t>(ptr);

    for(mm::slab::slab* slab = slab_list; slab; slab = slab->next_slab){
        if(slab->free(loc)) return;
    }
}