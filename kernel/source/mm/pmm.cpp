#include <Sigma/mm/pmm.h>
#include <Sigma/proc/elf.h>

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
        return (this->base == rhs.base) && (this->n_pages == rhs.n_pages);
    }

    bool operator <(rle_stack_entry &rhs){
        return (this->base < rhs.base);
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
    size_t i = 0;
    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++)
        debug_printf("  Entry %d: Start: %x, Length: %x [%x]\n", i++, entry->base, entry->n_pages, (entry->n_pages * mm::pmm::block_size));
}

static void sort_stack();

void mm::pmm::init(boot::boot_protocol* boot_protocol){
    pmm_global_mutex.lock();

    stack_base = reinterpret_cast<rle_stack_entry*>(kernel_end);
    stack_pointer = stack_base;

    uint64_t n_blocks = 0;
    {
        auto* ent = reinterpret_cast<multiboot_tag_mmap*>(boot_protocol->mmap);
        for(multiboot_memory_map_t* entry = ent->entries; (uint64_t)entry < ((uint64_t)ent + ent->size); entry++)
            if(entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                n_blocks += (entry->len / mm::pmm::block_size);

        printf("Detected Memory: %dmb\n", (n_blocks * 4) / 1024);
    }

    uint64_t worst_case_size = (n_blocks * sizeof(rle_stack_entry));
    kernel_end += worst_case_size;

    stack_top = reinterpret_cast<rle_stack_entry*>(kernel_end);

    mbd_start = boot_protocol->reserve_start & ~(mm::pmm::block_size - 1);
    mbd_end = ((boot_protocol->reserve_start + boot_protocol->reserve_length) & ~(mm::pmm::block_size - 1)) + mm::pmm::block_size;
    initrd_start = boot_protocol->kernel_initrd_ptr;
    initrd_end = boot_protocol->kernel_initrd_size + initrd_start;

    const auto [symtab_range, strtab_range] = proc::elf::get_symbol_pmm_exclusion_zones();
    const auto [symtab_base, symtab_size] = symtab_range;
    const auto [strtab_base, strtab_size] = strtab_range;

    auto* ent = reinterpret_cast<multiboot_tag_mmap*>(boot_protocol->mmap);
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

        if(entry->type != MULTIBOOT_MEMORY_AVAILABLE)
            continue;
        
        // TODO: Make this work on real hw
        // If it is below 1MiB just skip it
        if((entry->addr + entry->len) < (1024 * 1024))
            continue;
        else if(entry->addr < (1024 * 1024))
            push({.base = (1024 * 1024), .n_pages = ((entry->len - ((1024 * 1024) - entry->addr)) / mm::pmm::block_size)});
        else
            push({.base = entry->addr, .n_pages = (entry->len / mm::pmm::block_size)});
    }

    sort_stack();

    auto reserve_block = [](uint64_t addr){
        for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
            if(addr >= entry->base && addr <= (entry->base + (entry->n_pages * mm::pmm::block_size)) && entry->n_pages != 0){
                // Replace the entry and push the other one
                uint64_t low_offset = addr - entry->base;
                uint64_t high_offset = (entry->base + (entry->n_pages * mm::pmm::block_size)) - (addr + mm::pmm::block_size);

                auto first = rle_stack_entry{.base = entry->base, .n_pages = (low_offset / mm::pmm::block_size)};
                auto second = rle_stack_entry{.base = (addr + mm::pmm::block_size), .n_pages = (high_offset / mm::pmm::block_size)};

                if(first.n_pages == 0){
                    *entry = second;
                } else {
                    *entry = first;
                    if(second.n_pages)
                        push(second);
                }
                break;
            }
        }
    };

    for(uint64_t addr = (kernel_start - KERNEL_VBASE); addr <= (kernel_end - KERNEL_VBASE); addr += mm::pmm::block_size)
        reserve_block(addr);

    for(uint64_t addr = (mbd_start - KERNEL_VBASE); addr <= (mbd_end - KERNEL_VBASE); addr += mm::pmm::block_size)
        reserve_block(addr);

    for(uint64_t addr = initrd_start; addr <= initrd_end; addr += mm::pmm::block_size)
        reserve_block(addr);

    for(uint64_t addr = ALIGN_DOWN(symtab_base, mm::pmm::block_size); addr <= ALIGN_UP(symtab_base + symtab_size, mm::pmm::block_size); addr += mm::pmm::block_size)
        reserve_block(addr);

    for(uint64_t addr = ALIGN_DOWN(strtab_base, mm::pmm::block_size); addr <= ALIGN_UP(strtab_base + strtab_size, mm::pmm::block_size); addr += mm::pmm::block_size)
        reserve_block(addr);

    sort_stack(); // Cleanup things that might've been left behind by reserve_block()

    pmm_global_mutex.unlock();

    // TODO: Remove this hack and just ignore any mem under 1MiB
    for(int i = 0; i < 10; i++)
        mm::pmm::alloc_block();
}

NODISCARD_ATTRIBUTE
void* mm::pmm::alloc_block(){
    std::lock_guard guard{pmm_global_mutex};

    rle_stack_entry ent = pop();
    while(ent.n_pages == 0) 
        ent = pop();
    
    uint64_t addr = ent.base;
    ent.base += mm::pmm::block_size;
    ent.n_pages--;
    if(ent.n_pages != 0) 
        push(ent);

    return reinterpret_cast<void*>(addr);
}

NODISCARD_ATTRIBUTE
void* mm::pmm::alloc_n_blocks(size_t n){
    std::lock_guard guard{pmm_global_mutex};

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

    return reinterpret_cast<void*>(base);
}

void mm::pmm::free_block(void* block){
    std::lock_guard guard{pmm_global_mutex};

    auto addr = reinterpret_cast<uint64_t>(block);

    for(rle_stack_entry* entry = stack_base; entry < stack_pointer; entry++){
        if((addr + mm::pmm::block_size) == entry->base){
            // We're just under this entry
            entry->base -= mm::pmm::block_size;
            entry->n_pages++;
            return;
        } else if((entry->base + (entry->n_pages * mm::pmm::block_size)) == addr){
            // We're just above this entry
            entry->n_pages++;
            return;
        }
    }

    // We're not consecutive to any entry, add our own
    push({.base = addr, .n_pages = 1});

    return;
}

static void sorted_insert(rle_stack_entry x){
    if ((stack_base == stack_pointer) || (x < *stack_pointer))
    { 
        push(x); 
        return; 
    } 
  
    // If top is greater, remove the top item and recur 
    auto temp = pop(); 
    sorted_insert(x); 
  
    // Put back the top item removed earlier 
    push(temp); 
} 

// Function to sort stack 
static void sort_stack() {
    if(stack_base != stack_pointer){
        auto entry = pop();
  
        sort_stack(); 

        if(entry.n_pages != 0)
            sorted_insert(entry); 
    }
} 