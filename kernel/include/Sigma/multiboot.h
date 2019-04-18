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
    }



    private:
        void* mbd;
        uint64_t magic;
};

#endif