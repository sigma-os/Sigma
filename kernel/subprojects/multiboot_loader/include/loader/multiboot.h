#ifndef SIGMA_LOADER_MULTIBOOT
#define SIGMA_LOADER_MULTIBOOT

#include <loader/common.h>

#include <loader/3rdparty/multiboot.h>

namespace loader
{
    class multiboot {
        public:
        multiboot(void* mbd, uint64_t magic): mbd(mbd), magic(magic){
            if(magic != MULTIBOOT2_BOOTLOADER_MAGIC){
                loader::common::debug_printf("Multiboot header magic not correct\n");
                loader::common::abort();
            }

            this->parse_mbd();
        }


        uint64_t get_memsize_mb(){
            return ((((this->mem_low * 1024) + (this->mem_high * 1024)) / 1024) / 1024) + 1;
        }

        uint64_t get_rsdp(){
            return reinterpret_cast<uint64_t>(rsdp);
        }

        multiboot_tag_mmap* get_mmap_entry(){
            return reinterpret_cast<multiboot_tag_mmap*>(mmap_entry);
        }

        uint64_t get_mbd_size(){
            uint32_t* base = reinterpret_cast<uint32_t*>(this->mbd);
            uint64_t total_size = *base;
            return total_size;
        }

        uint64_t get_mbd_ptr(){
            return reinterpret_cast<uint64_t>(this->mbd);
        }

        uint64_t* get_elf_sections(){
            return this->elf_sections;
        }

        uint64_t get_elf_n_sections(){
            return this->n_elf_sections;
        }

        uint64_t get_initrd_ptr(){
            return this->initrd_ptr;
        }

        uint64_t get_initrd_size(){
            return this->initrd_size;
        }

        private:
        uint32_t mem_low, mem_high;
        uint64_t* rsdp = nullptr;
        uint64_t* elf_sections = nullptr;
        uint64_t n_elf_sections = 0;

        uint64_t initrd_ptr, initrd_size;

        uint64_t mmap_entry;

        void parse_mbd();
        void* mbd;
        uint64_t magic;
    };
} // namespace loader

#endif