#include <loader/multiboot.h>

static int memcmp(const void* s1, const void* s2, size_t n){
    const uint8_t* a = (const uint8_t*)s1;
    const uint8_t* b = (const uint8_t*)s2;

    for(size_t i = 0; i < n; i++){
        if(a[i] < b[i]) return -1;
        else if(b[i] < a[i]) return 1;
    }

    return 0;
}

void loader::multiboot::parse_mbd(){
    uint32_t* base = static_cast<uint32_t*>(this->mbd);
    uint64_t total_size = *base;

    uint64_t add_size = 0;

    for(uint64_t i = 8/*skip fixed part*/; i < total_size; i += add_size){
        uint32_t* type = reinterpret_cast<uint32_t*>((uint64_t)base + i);
        if(*type == MULTIBOOT_TAG_TYPE_END) break;

        


        uint32_t* size = reinterpret_cast<uint32_t*>((uint64_t)base + i + 4);
        add_size = *size;

        if((add_size % 8)!= 0) add_size += (8 - add_size % 8); // Align 8byte

        switch (*type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
            {
                auto info = reinterpret_cast<multiboot_tag_string*>(type);
                this->kernel_cmdline = info->string;
            }
            break;

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

            case MULTIBOOT_TAG_TYPE_MODULE:
            {
                multiboot_tag_module* info = reinterpret_cast<multiboot_tag_module*>(type);

                if(memcmp(info->cmdline, "initrd.tar", 10) == 0){
                    this->initrd_ptr = info->mod_start;
                    this->initrd_size = (info->mod_end - info->mod_start);
                }
            }
            break;
        
            default:
                break;
        }
    }
}