#include <Sigma/mm/pmm.h>

C_LINKAGE uint64_t _kernel_start;
C_LINKAGE uint64_t _kernel_end;

uint64_t kernel_start = reinterpret_cast<uint64_t>(&_kernel_start);
uint64_t kernel_end = reinterpret_cast<uint64_t>(&_kernel_end);

uint64_t* bitmap;
uint64_t bitmap_size;

uint64_t n_blocks;

x86_64::spinlock::mutex pmm_mutex = x86_64::spinlock::mutex();

static void bitmap_set (int bit) {
  bitmap[bit / 64] |= (1 << (bit % 64));
}

static bool bitmap_test (int bit) {
	return bitmap[bit / 64] &  (1 << (bit % 64));
}

static void enable_region(uint64_t base, uint64_t size){
    uint64_t align = base / mm::pmm::block_size;
    uint64_t blocks = size / mm::pmm::block_size;

    for(; blocks > 0; blocks--){
        bitops<uint64_t>::bit_clear(bitmap[align / 64], align % 64);

        align++;
    }

    return;

    /*uint64_t bit = base / 0x1000;
    for(uint64_t n = (size / 0x1000); n > 0; n--){
        bitops<uint64_t>::bit_clear(bitmap[bit / 64], bit % 64);
        bit++;
    }*/
}

static void disable_region(uint64_t base, uint64_t size){
    uint64_t align = base / mm::pmm::block_size;
    uint64_t blocks = size / mm::pmm::block_size;

    for(; blocks > 0; blocks--){
        bitops<uint64_t>::bit_set(bitmap[align / 64], align % 64);

        align++;
    }

    return;
}

void mm::pmm::init(multiboot& mbi){
    x86_64::spinlock::acquire(&pmm_mutex);

    n_blocks = (mbi.get_memsize_mb() * 0x100000) / 0x1000;

    bitmap_size = n_blocks / 64;
 
    bitmap = reinterpret_cast<uint64_t*>(kernel_end);
    kernel_end += bitmap_size; // Readjust for bitmap, it comes directly after the kernel


    memset(reinterpret_cast<void*>(bitmap), 0xF, n_blocks / 8); // Reserve all blocks

    multiboot_tag_mmap* ent = mbi.get_mmap_entry();

    for(multiboot_memory_map_t* entry = ent->entries; (uintptr_t)entry < (uintptr_t)((uint64_t)ent + ent->size); entry++){// += ent->entry_size){
        debug_printf("[PMM]: MMAP Entry: Base: %x, Length: %x, Type: ", entry->addr, entry->len);
        switch (entry->type)
        {
            case MULTIBOOT_MEMORY_AVAILABLE:
                debug_printf("Available\n");

                debug_printf("ACTIVE: %x, %x\n", entry->addr, entry->len);

                enable_region(entry->addr, entry->len);//(entry->len & 0xFFFFFFFFFFFFF000));
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                debug_printf("Reserved\n");
                disable_region(entry->addr, entry->len);
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                debug_printf("ACPI Reclaimable\n");
                disable_region(entry->addr, entry->len);
                break;

            case MULTIBOOT_MEMORY_NVS:
                debug_printf("Non Volatile Storage\n");
                disable_region(entry->addr, entry->len);
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                debug_printf("BADRAM\n");
                disable_region(entry->addr, entry->len);
                break;
            
            default:
                debug_printf("Unkown RAM type\n");
                disable_region(entry->addr, entry->len);
                break;
        }
    }

    bitops<uint64_t>::bit_set(bitmap[0], 0); // Reserve block 1, it stores the IVT and BDA
    bitops<uint64_t>::bit_set(bitmap[0], 1); // SMP Trampoline code
    bitops<uint64_t>::bit_set(bitmap[0], 2); // SMP Trampoline PML4
    bitops<uint64_t>::bit_set(bitmap[0], 3); // SMP Trampoline PDPT
    bitops<uint64_t>::bit_set(bitmap[0], 4); // SMP Trampoline PD
    bitops<uint64_t>::bit_set(bitmap[0], 5); // SMP Extra

    uint64_t kernel_start_phys = (kernel_start - KERNEL_VBASE);
    uint64_t kernel_end_phys = (kernel_end - KERNEL_VBASE);

    uint64_t mbi_phys = (reinterpret_cast<uint64_t>(&mbi) - KERNEL_VBASE);

    disable_region(kernel_start_phys, (kernel_end_phys - kernel_start_phys));
    disable_region(mbi_phys, mbi.get_mbd_size());

    x86_64::spinlock::release(&pmm_mutex);
}

static uint64_t first_free(){
    for(uint64_t i = 0; i < (n_blocks / 64); i++){
        if(bitmap[i] != 0xFFFFFFFFFFFFFFFF){ // Test if this entry is full
            for(uint64_t j = 0; j < 64; j++){
                uint64_t bit = 1ULL << j;
                if(!(bitmap[i] & bit)) return i * 64 + j;
            }
        }
    }

    return (uint64_t)-1;
}

static uint64_t first_free_n(size_t n_frames){
    if(n_frames == 0) return -2;

    if(n_frames == 1) return first_free();

    for(uint64_t i = 0; i < (n_blocks / 64); i++){
        if(bitmap[i] != 0xFFFFFFFFFFFFFFFF){ // Test if this entry is full
            for(uint64_t j = 0; j < 64; j++){
                uint64_t bit = 1ULL << j;
                if(!(bitmap[i] & bit)){
                    uint64_t starting_bit = i * 64;

                    starting_bit += bit;

                    uint64_t free = 0;
                    for(uint64_t count = 0; count <= n_frames; count++){
                        if(!bitmap_test(starting_bit + count)){
                            free++;
                        }

                        if(free == n_frames){
                            return i * 64 + j;
                        }
                    }
                }
            }
        }
    }

    return (uint64_t)-1;
}

void* mm::pmm::alloc_block(){
    x86_64::spinlock::acquire(&pmm_mutex);

    uint64_t block_bit = first_free();

    if(block_bit == (uint64_t)-1){
        printf("[PMM]: Out of blocks!\n");
        x86_64::spinlock::release(&pmm_mutex);
        abort();
    }

    uint64_t address = block_bit * mm::pmm::block_size;
    bitops<uint64_t>::bit_set(bitmap[block_bit / 64], block_bit % 64);

    x86_64::spinlock::release(&pmm_mutex);

    return reinterpret_cast<void*>(address);
}

void* mm::pmm::alloc_n_blocks(size_t n_blocks){
    x86_64::spinlock::acquire(&pmm_mutex);

    uint64_t block_bit = first_free_n(n_blocks);

    if(block_bit == (uint64_t)-1){
        printf("[PMM]: Out of blocks!\n");
        x86_64::spinlock::release(&pmm_mutex);
        abort();
    }

    if(block_bit == (uint64_t)-2){
        printf("[PMM]: Failed to allocate %d consecutive blocks!\n", n_blocks);
        x86_64::spinlock::release(&pmm_mutex);
        abort();
    }

    uint64_t address = block_bit * mm::pmm::block_size;
    for(uint64_t i = 0; i < n_blocks; i++){
        bitmap_set(block_bit + i);
    }
    

    x86_64::spinlock::release(&pmm_mutex);

    return reinterpret_cast<void*>(address);
}

void mm::pmm::free_block(void* block){
    x86_64::spinlock::acquire(&pmm_mutex);
    uint64_t addr = reinterpret_cast<uint64_t>(block);

    uint64_t bit = addr / mm::pmm::block_size;
    bitops<uint64_t>::bit_clear(bitmap[bit / 64], bit % 64);

    x86_64::spinlock::release(&pmm_mutex);
}

void mm::pmm::free_blocks(void* blocks, size_t n_blocks) {
    x86_64::spinlock::acquire(&pmm_mutex);
	uint64_t addr = reinterpret_cast<uint64_t>(blocks);
	uint64_t bit = addr / mm::pmm::block_size;

	for (uint32_t i = 0; i< n_blocks; i++) bitops<uint64_t>::bit_clear(bitmap[bit / 64], bit % 64);
    x86_64::spinlock::release(&pmm_mutex);
}