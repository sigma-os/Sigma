#ifndef SIGMA_KERNEL_MULTIBOOT
#define SIGMA_KERNEL_MULTIBOOT

#include <Sigma/common.h>

#include <Sigma/3rdparty/multiboot.h>

#include <klibc/stdio.h>
#include <klibc/stdlib.h>

class multiboot {
    public:
    multiboot(void* mbd, uint64_t magic): mbd(mbd), magic(magic){
        if(magic != MULTIBOOT2_BOOTLOADER_MAGIC){
            printf("Header magic gotten %x is not equal to multiboot header magic %x\n", this->magic, MULTIBOOT2_BOOTLOADER_MAGIC);
            abort();
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

    private:
    uint32_t mem_low, mem_high;
    uint64_t* rsdp = nullptr;

    uint64_t mmap_entry;

    void parse_mbd();
    void* mbd;
    uint64_t magic;
};

#endif