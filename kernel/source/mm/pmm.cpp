#include <Sigma/mm/pmm.h>

C_LINKAGE uint64_t _kernel_start;
C_LINKAGE uint64_t _kernel_end;

uint64_t kernel_start = reinterpret_cast<uint64_t>(&_kernel_start);
uint64_t kernel_end = reinterpret_cast<uint64_t>(&_kernel_end);

uint64_t mbd_start, initrd_start;
uint64_t mbd_end, initrd_end;

auto pmm_global_mutex = x86_64::spinlock::mutex();



struct rle_stack_entry {
    rle_stack_entry(uint64_t base, uint64_t n_pages): base(base), n_pages(n_pages) {}
    uint64_t base;
    uint64_t n_pages;

    bool operator== (rle_stack_entry &rhs){
        if((this->base == rhs.base) && (this->n_pages == rhs.n_pages)) return true;
        return false;
    }

    bool operator <(rle_stack_entry &rhs){
        if(this->base < rhs.base) return true;
        return false;
    }
};

rle_stack_entry* stack_base;
rle_stack_entry* stack_pointer;


//TODO: Check for stack undeflow and overflow
static rle_stack_entry pop(){
    stack_pointer--;
    rle_stack_entry ent = *stack_pointer;
    return ent;
}

static void push(rle_stack_entry entry){
    *stack_pointer++ = entry;
}

void mm::pmm::print_stack(){
    debug_printf("Starting PMM Stack dump\n");
    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
        debug_printf("  Entry: Start: %x, Length: %x\n", entry->base, (entry->n_pages * mm::pmm::block_size));
    }
}

static void sort_stack();

void mm::pmm::init(boot::boot_protocol* boot_protocol){
    x86_64::spinlock::acquire(&pmm_global_mutex);

    stack_base = reinterpret_cast<rle_stack_entry*>(kernel_end);
    stack_pointer = stack_base;

    uint64_t n_blocks = (boot_protocol->memsize * 0x100);
    uint64_t worst_case_size = (n_blocks * sizeof(rle_stack_entry));
    kernel_end += worst_case_size;

    mbd_start = boot_protocol->reserve_start;
    mbd_end = (boot_protocol->reserve_start + boot_protocol->reserve_length);
    initrd_start = boot_protocol->kernel_initrd_ptr;
    initrd_end = boot_protocol->kernel_initrd_size + initrd_start;

    multiboot_tag_mmap* ent = reinterpret_cast<multiboot_tag_mmap*>(boot_protocol->mmap);
    for(multiboot_memory_map_t* entry = ent->entries; (uint64_t)entry < ((uint64_t)ent + ent->size); entry++){// += ent->entry_size){
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            push({entry->addr, (entry->len / mm::pmm::block_size)});
        }
    }

    sort_stack();

    x86_64::spinlock::release(&pmm_global_mutex);

    // TODO: Remove this hack and just ignore any memory under 1mb
    for(uint8_t i = 0; i < 10; i++){ // Reserve first 10 blocks for BIOS stuff and Trampoline code
        mm::pmm::alloc_block();
    }
}

void* mm::pmm::alloc_block(){
    x86_64::spinlock::acquire(&pmm_global_mutex);
    rle_stack_entry ent = pop();
    while(ent.n_pages == 0) ent = pop();
    uint64_t addr = ent.base;
    ent.base += mm::pmm::block_size;
    ent.n_pages--;
    if(ent.n_pages != 0) push(ent);

    if(((addr >= (kernel_start - KERNEL_VBASE)) && (addr <= (kernel_end - KERNEL_VBASE))) || ((addr >= (mbd_start - KERNEL_VBASE) && addr <= (mbd_end - KERNEL_VBASE))) || ((addr >= (initrd_start) && addr <= (initrd_end)))){
        // Addr is in kernel, multiboot info or initrd so just ignore it and get a new one
        x86_64::spinlock::release(&pmm_global_mutex);
        return mm::pmm::alloc_block();
    }
    x86_64::spinlock::release(&pmm_global_mutex);
    return reinterpret_cast<void*>(addr);
}

void* mm::pmm::alloc_n_blocks(size_t n){
    x86_64::spinlock::acquire(&pmm_global_mutex);
    uint64_t base = 0;
    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
        if(entry->n_pages >= n){
            // Found entry that is big enough to hold us
            base = entry->base;
            entry->base += (mm::pmm::block_size * n);
            entry->n_pages -= n;
        }
    }
    if(base == 0){
        PANIC("[PMM]: Out of memory");
    }

    if(((base >= (kernel_start - KERNEL_VBASE)) && (base <= (kernel_end - KERNEL_VBASE))) || ((base >= (mbd_start - KERNEL_VBASE) && base <= (mbd_end - KERNEL_VBASE))) || ((base >= (initrd_start) && base <= (initrd_end)))){
        // Addr is in kernel or multiboot info so just ignore it and get a new one
        x86_64::spinlock::release(&pmm_global_mutex);
        return mm::pmm::alloc_n_blocks(n);
    }
    x86_64::spinlock::release(&pmm_global_mutex);
    return reinterpret_cast<void*>(base);
}

void mm::pmm::free_block(void* block){
    x86_64::spinlock::acquire(&pmm_global_mutex);

    uint64_t addr = reinterpret_cast<uint64_t>(block);

    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
        if((addr + mm::pmm::block_size) == entry->base){
            // We're just under this entry
            entry->base -= mm::pmm::block_size;
            entry->n_pages++;
            x86_64::spinlock::release(&pmm_global_mutex);
            return;
        } else if((entry->base + mm::pmm::block_size) == addr){
            // We're just above this entry
            entry->n_pages++;
            x86_64::spinlock::release(&pmm_global_mutex);
            return;
        }
    }

    // We're not consecutive to any entry, add our own
    push({addr, 1});

    x86_64::spinlock::release(&pmm_global_mutex);
    return;
}

static void sorted_insert(rle_stack_entry x) 
{ 
    // Base case: Either stack is empty or newly inserted 
    // item is greater than top (more than all existing) 
    if ((stack_base == stack_pointer) || (x < *stack_pointer)) 
    { 
        push(x); 
        return; 
    } 
  
    // If top is greater, remove the top item and recur 
    rle_stack_entry temp = pop(); 
    sorted_insert(x); 
  
    // Put back the top item removed earlier 
    push(temp); 
} 
  
// Function to sort stack 
static void sort_stack() 
{ 
    if(stack_base != stack_pointer){
        rle_stack_entry x = pop(); 
  
        sort_stack(); 

        sorted_insert(x); 
    }
} 