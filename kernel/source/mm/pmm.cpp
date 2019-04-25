#include <Sigma/mm/pmm.h>

C_LINKAGE uint64_t _kernel_start;
C_LINKAGE uint64_t _kernel_end;

uint64_t kernel_start = reinterpret_cast<uint64_t>(&_kernel_start);
uint64_t kernel_end = reinterpret_cast<uint64_t>(&_kernel_end);

uint64_t* bitmap;
uint64_t bitmap_size;

uint64_t n_blocks;

static void enable_region(uint64_t base, uint64_t size){
    uint64_t align = base / 0x1000;
    uint64_t blocks = size / 0x1000;

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
    uint64_t bit = base / 0x1000;
    for(uint64_t n = (size / 0x1000); n > 0; n--){
        bitops<uint64_t>::bit_set(bitmap[bit / 64], bit % 64);
        bit++;
    }
}

void mm::pmm::init(multiboot& mbi){
    n_blocks = (mbi.get_memsize_mb() * 0x100000) / 0x1000;

    bitmap_size = n_blocks / 64;
 
    bitmap = reinterpret_cast<uint64_t*>(kernel_end);
    kernel_end += bitmap_size; // Readjust for bitmap, it comes directly after the kernel


    memset(reinterpret_cast<void*>(bitmap), 0xFF, n_blocks / 8); // Reserve all blocks

    multiboot_tag_mmap* ent = mbi.get_mmap_entry();

    for(multiboot_memory_map_t* entry = ent->entries; (uintptr_t)entry < (uintptr_t)((uint64_t)ent + ent->size); entry++){// += ent->entry_size){
        debug_printf("[PMM]: MMAP Entry: Base: %x, Length: %x, Type: ", entry->addr, entry->len);
        switch (entry->type)
        {
            case MULTIBOOT_MEMORY_AVAILABLE:
                debug_printf("Available\n");

                enable_region(entry->addr, entry->len);//(entry->len & 0xFFFFFFFFFFFFF000));
                break;

            case MULTIBOOT_MEMORY_RESERVED:
                debug_printf("Reserved\n");
                break;

            case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                debug_printf("ACPI Reclaimable\n");
                break;

            case MULTIBOOT_MEMORY_NVS:
                debug_printf("Non Volatile Storage\n");
                break;

            case MULTIBOOT_MEMORY_BADRAM:
                debug_printf("BADRAM\n");
                break;
            
            default:
                debug_printf("Unkown RAM type\n");
                break;
        }
    }

    bitops<uint64_t>::bit_set(bitmap[0], 0); // Reserve block 1, it stores the IVT and BDA

    uint64_t kernel_start_phys = (kernel_start - KERNEL_VBASE);
    uint64_t kernel_end_phys = (kernel_end - KERNEL_VBASE);

    uint64_t mbi_phys = (reinterpret_cast<uint64_t>(&mbi) - KERNEL_VBASE);

    disable_region(kernel_start_phys, (kernel_end_phys - kernel_start_phys));
    disable_region(mbi_phys, mbi.get_mbd_size());
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

void* mm::pmm::alloc_block(){
    uint64_t block_bit = first_free();

    if(block_bit == (uint64_t)-1){
        printf("[PMM]: Out of blocks!\n");
        abort();
    }

    uint64_t address = block_bit * 0x1000;
    bitops<uint64_t>::bit_set(bitmap[block_bit / 64], block_bit % 64);

    return reinterpret_cast<void*>(address);
}

void mm::pmm::free_block(void* block){
    uint64_t addr = reinterpret_cast<uint64_t>(block);

    uint64_t bit = addr / 0x1000;
    bitops<uint64_t>::bit_clear(bitmap[bit / 64], bit % 64);
}