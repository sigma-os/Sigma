#include <Sigma/mm/pmm.h>

C_LINKAGE uint64_t _kernel_start;
C_LINKAGE uint64_t _kernel_end;

uint64_t kernel_start = reinterpret_cast<uint64_t>(&_kernel_start);
uint64_t kernel_end = reinterpret_cast<uint64_t>(&_kernel_end);

static uint64_t mbd_start, initrd_start;
static uint64_t mbd_end, initrd_end;

static auto pmm_global_mutex = x86_64::spinlock::mutex();



struct rle_stack_entry {
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

static rle_stack_entry* stack_base;
static rle_stack_entry* stack_pointer;
static rle_stack_entry* stack_top;


static rle_stack_entry pop(){
    stack_pointer--;
    if(stack_pointer == (stack_base - 1))
        PANIC("[PMM]: RLE Stack underflow");

    rle_stack_entry ent = *stack_pointer;
    return ent;
}

static void push(rle_stack_entry entry){
    if((stack_pointer + 2) == stack_top)
        PANIC("[PMM]: RLE Stack overflow");
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
    pmm_global_mutex.lock();

    stack_base = reinterpret_cast<rle_stack_entry*>(kernel_end);
    stack_pointer = stack_base;

    uint64_t n_blocks = 0;
    {
        multiboot_tag_mmap* ent = reinterpret_cast<multiboot_tag_mmap*>(boot_protocol->mmap);
        for(multiboot_memory_map_t* entry = ent->entries; (uint64_t)entry < ((uint64_t)ent + ent->size); entry++)
            if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                n_blocks += (entry->len / mm::pmm::block_size);

        printf("Detected Memory: %dmb\n", (n_blocks * 4) / 1024);
    }

    uint64_t worst_case_size = (n_blocks * sizeof(rle_stack_entry));
    kernel_end += worst_case_size;

    stack_top = reinterpret_cast<rle_stack_entry*>(kernel_end);

    mbd_start = boot_protocol->reserve_start;
    mbd_end = (boot_protocol->reserve_start + boot_protocol->reserve_length);
    initrd_start = boot_protocol->kernel_initrd_ptr;
    initrd_end = boot_protocol->kernel_initrd_size + initrd_start;

    multiboot_tag_mmap* ent = reinterpret_cast<multiboot_tag_mmap*>(boot_protocol->mmap);
    for(multiboot_memory_map_t* entry = ent->entries; (uint64_t)entry < ((uint64_t)ent + ent->size); entry++){
        debug_printf("[e820]: Base: %x, Len: %x, Type: ", entry->addr, entry->len);
        switch (entry->type)
        {
        case MULTIBOOT_MEMORY_AVAILABLE:
            debug_printf("Available\n");
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            debug_printf("Reserved\n");
            break;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            debug_printf("ACPI Reclaimable\n");
            break;
        case MULTIBOOT_MEMORY_NVS:
            debug_printf("NVS\n");
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            debug_printf("Badram\n");
            break;
        default:
            debug_printf("Unknown\n");
            break;
        }

        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            // TODO: Make this work on real hw
            // If it is below 1MiB just skip it
            /*if(entry->addr + entry->len < (1024 * 1024))
                continue;
            else if(entry->addr < (1024 * 1024))
                push({.base = (1024 * 1024), .n_pages = ((entry->len - ((1024 * 1024) - entry->addr)) / mm::pmm::block_size)});
            else*/
                push({.base = entry->addr, .n_pages = (entry->len / mm::pmm::block_size)});
        }
    }

    sort_stack();

    pmm_global_mutex.unlock();

    // TODO: Remove this hack and just ignore any mem under 1MiB
    for(int i = 0; i < 10; i++)
        mm::pmm::alloc_block();
}

NODISCARD_ATTRIBUTE
void* mm::pmm::alloc_block(){
    pmm_global_mutex.lock();
    rle_stack_entry ent = pop();
    while(ent.n_pages == 0) ent = pop();
    uint64_t addr = ent.base;
    ent.base += mm::pmm::block_size;
    ent.n_pages--;
    if(ent.n_pages != 0) push(ent);

    if(((addr >= (kernel_start - KERNEL_VBASE)) && (addr <= (kernel_end - KERNEL_VBASE))) || ((addr >= (mbd_start - KERNEL_VBASE) && addr <= (mbd_end - KERNEL_VBASE))) || ((addr >= (initrd_start) && addr <= (initrd_end)))){
        // Addr is in kernel, multiboot info or initrd so just ignore it and get a new one
        pmm_global_mutex.unlock();
        return mm::pmm::alloc_block();
    }
    pmm_global_mutex.unlock();
    return reinterpret_cast<void*>(addr);
}

NODISCARD_ATTRIBUTE
void* mm::pmm::alloc_n_blocks(size_t n){
    pmm_global_mutex.lock();
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
        pmm_global_mutex.unlock();
        return mm::pmm::alloc_n_blocks(n);
    }
    pmm_global_mutex.unlock();
    return reinterpret_cast<void*>(base);
}

void mm::pmm::free_block(void* block){
    std::lock_guard guard{pmm_global_mutex};

    uint64_t addr = reinterpret_cast<uint64_t>(block);

    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
        if((addr + mm::pmm::block_size) == entry->base){
            // We're just under this entry
            entry->base -= mm::pmm::block_size;
            entry->n_pages++;
            return;
        } else if((entry->base + mm::pmm::block_size) == addr){
            // We're just above this entry
            entry->n_pages++;
            return;
        }
    }

    // We're not consecutive to any entry, add our own
    push({.base = addr, .n_pages = 1});

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