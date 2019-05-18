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

            case MULTIBOOT_TAG_TYPE_MMAP:
                this->mmap_entry = reinterpret_cast<uint64_t>(type);
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

            case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
            {
                multiboot_tag_elf_sections* info = reinterpret_cast<multiboot_tag_elf_sections*>(type);
                /*for(uint64_t i = 0; i < info->num; i++){
                    multiboot_elf::Elf32_Shdr* shdr = reinterpret_cast<multiboot_elf::Elf32_Shdr*>(reinterpret_cast<uint64_t>(info->sections) + (i * info->entsize));
                    if(shdr->sh_flags & 0x2){
                        printf("Section: base %x, len: %x, type: %x, ALLOCATED ", shdr->sh_addr, shdr->sh_size, shdr->sh_type);
                        if(shdr->sh_flags & 0x1) printf("WRITABLE ");
                        if(shdr->sh_flags & 0x4) printf("EXECUTABLE ");
                        printf("\n");
                    }
                }*/
                if(this->elf_sections == nullptr){
                    this->elf_sections = reinterpret_cast<uint64_t*>(info->sections);
                    this->n_elf_sections = info->num;
                }
            }
            break;
        
            default:
                break;
        }
    }

    debug_printf("[MULTIBOOT]: Parsed info structure\n");
}