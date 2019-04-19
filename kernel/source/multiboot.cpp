#include <Sigma/multiboot.h>

void multiboot::parse_mbd(){

    debug_printf("[MULTIBOOT]: Parsing info structure\n");

    uint32_t* base = reinterpret_cast<uint32_t*>(this->mbd);
    uint64_t total_size = *base;

    uint64_t add_size = 0;

    for(uint64_t i = 8/*skip fixed part*/; i < total_size; i += add_size){
        uint32_t* type = reinterpret_cast<uint32_t*>((uint64_t)base + i);
        if(*type == MULTIBOOT_TAG_TYPE_END) break;

        


        uint32_t* size = reinterpret_cast<uint32_t*>((uint64_t)base + i + 4);
        add_size = *size;

        if((add_size % 8)!= 0) add_size += (8 - add_size % 8); // Align 8byte

        debug_printf("Found tag with type %i, size %i\n", *type, *size);

        switch (*type)
        {
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
            {
                multiboot_tag_basic_meminfo* info = reinterpret_cast<multiboot_tag_basic_meminfo*>(type);
                this->mem_low = info->mem_lower;
                this->mem_high = info->mem_upper;
            }
            break;

            case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            {
                multiboot_tag_old_acpi* info = reinterpret_cast<multiboot_tag_old_acpi*>(type);
                if(rsdp == nullptr) this->rsdp = reinterpret_cast<uint64_t*>(info->rsdp);
            }
            break;

            case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            {
                multiboot_tag_new_acpi* info = reinterpret_cast<multiboot_tag_new_acpi*>(type);
                if(rsdp == nullptr) this->rsdp = reinterpret_cast<uint64_t*>(info->rsdp);
            }
            break;
        
            default:
                break;
        }
    }

    debug_printf("[MULTIBOOT]: Parsed info structure\n");
}