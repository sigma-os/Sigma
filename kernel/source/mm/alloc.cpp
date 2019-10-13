#include <Sigma/mm/alloc.h>

#include <Sigma/debug.h>

static x86_64::spinlock::mutex alloc_global_mutex = x86_64::spinlock::mutex();

static alloc::header* head = nullptr;
static alloc::header* tail = nullptr;

NODISCARD_ATTRIBUTE
static alloc::header* get_free_block(size_t size){
    // Basic First Fit algorithm
    alloc::header* curr = head;
    while(curr){
        if((curr->is_free == true) && (curr->size >= size)) return curr;
        curr = curr->next;
    }
    return nullptr;
}

static uint64_t current_heap_page_offset = 0xffffffffd0000000;
static uint64_t current_heap_offset = 0xffffffffd0000000;

NODISCARD_ATTRIBUTE
static uint64_t morecore(size_t size, uint64_t align = 8){
    current_heap_offset = ALIGN_UP(current_heap_offset, align);
    uint64_t ret = current_heap_offset;
    current_heap_offset += size;

    while(current_heap_page_offset <= current_heap_offset){
        void* block = mm::pmm::alloc_block();
        if(block == nullptr) return 0;
        mm::vmm::kernel_vmm::get_instance().map_page(reinterpret_cast<uint64_t>(block), current_heap_page_offset, map_page_flags_present | map_page_flags_writable | map_page_flags_global | map_page_flags_no_execute);
        memset(reinterpret_cast<void*>(current_heap_page_offset), 0, mm::pmm::block_size);
        current_heap_page_offset += mm::pmm::block_size;
    }

    return ret;
}

void alloc::print_list(){
	alloc::header *curr = head;
	debug_printf("head = %x, tail = %x\n", head, tail);
	while(curr) {
		debug_printf("addr = %x, size = %x, is_free=%d, next=%x, magic_low: %x, magic_high: %x\n", curr, curr->size, curr->is_free, curr->next, curr->magic_low, curr->magic_high);
        if(((uint64_t)curr->next < (0xffffffffd0000000 - 1) && ((uint64_t)curr->next != 0)) || (uint64_t)curr->next == 0xffffffffffffffff || !curr->check_magic()){
            debug_printf("Heap corruption detected in entry: addr: %x\n", curr);
            return;
        }
		curr = curr->next;
	}
}

NODISCARD_ATTRIBUTE
void* alloc::alloc(size_t size){
    if(size == 0) return nullptr;

    std::lock_guard guard{alloc_global_mutex};

    alloc::header* header = get_free_block(size);
    if(header){
        if(!header->check_magic()){
            debug_printf("[ALLOC]: Invalid header magic: allocation size: %x, header ptr: %x\n", size, header);
            alloc::print_list();
            debug::trace_stack(10);
            return nullptr;
        }

        // TODO: Carve rest of block up into other usable spaces

        header->is_free = false;
        return static_cast<void*>(header + 1);
    }

    size_t total_size = size + sizeof(alloc::header);
    uint64_t block = morecore(total_size, 64);
    if(block == 0){
        debug_printf("[ALLOC]: Failed to allocate block with size: %x\n", total_size);
        return nullptr;
    }

    header = reinterpret_cast<alloc::header*>(block);
    header->size = size;
    header->next = nullptr;
    header->set_magic();
    header->is_free = false;

    if(head == nullptr) head = header;
    if(tail) tail->next = header;
    tail = header;

    return static_cast<void*>(header + 1);
}

NODISCARD_ATTRIBUTE
void* alloc::alloc_a(size_t size, uint64_t align){
    std::lock_guard guard{alloc_global_mutex};

    size_t total_size = size + sizeof(alloc::header);
    uint64_t block = morecore(total_size, align);
    if(block == 0){
        debug_printf("[ALLOC]: Failed to allocate block with size: %x\n", total_size);
        return nullptr;
    }

    auto* header = reinterpret_cast<alloc::header*>(block);
    header->size = size;
    header->next = nullptr;
    header->set_magic();
    header->is_free = false;

    if(head == nullptr) head = header;
    if(tail) tail->next = header;
    tail = header;

    return static_cast<void*>(header + 1);
}

NODISCARD_ATTRIBUTE
void* alloc::realloc(void* ptr, size_t size){
    if(!ptr && size == 0) return nullptr;

    if(size == 0 && ptr != nullptr){
        alloc::free(ptr);
        return nullptr;
    }

    if(!ptr) return alloc::alloc(size);

    alloc::header* header = (static_cast<alloc::header*>(ptr) - 1);
    if(header->is_free){
        debug_printf("[ALLOC]: Tried to realloc free pointer\n");
        return nullptr;
    }

    if(!header->check_magic()){
        debug_printf("[ALLOC]: Invalid header magic: header ptr: %x\n", header);
        return nullptr;
    }
    if(header->size >= size) return ptr;

    void* ret = alloc::alloc(size);
    if(ret){
        memcpy(ret, ptr, header->size);
        alloc::free(ptr);
    }

    return ret;
}

void alloc::free(void* ptr){
    if(!ptr) return;
    std::lock_guard guard{alloc_global_mutex};
    alloc::header* header = (static_cast<alloc::header*>(ptr) - 1);

    if(!header->check_magic()){
        printf("[ALLOC]: Invalid header magic: header ptr: %x\n", header);
        alloc::print_list();
        debug::trace_stack(10);
        return;
    }

    if(header->is_free){
        debug_printf("[ALLOC]: Tried to double free ptr: %x!\n", ptr);
        return;
    }

    header->is_free = true;

    return;
}

void alloc::init(){
    for(size_t i = 0; i < 4; i++){ // Start with 4 pages
        mm::vmm::kernel_vmm::get_instance().map_page(reinterpret_cast<uint64_t>(mm::pmm::alloc_block()), current_heap_page_offset, map_page_flags_present | map_page_flags_writable | map_page_flags_global | map_page_flags_no_execute);
        current_heap_page_offset += mm::pmm::block_size;
    }
}