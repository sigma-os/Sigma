#include <Sigma/common.h>

#include <Sigma/arch/x86_64/gdt.h>

#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/multiboot.h>

extern "C" void kernel_main(void* multiboot_information, uint64_t magic){   

    multiboot mboot = multiboot(multiboot_information, magic);

    (void)(mboot);


    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();

    printf("Sigma: Successfully loaded GDT\n");

    printf("Sigma: reached end of kernel_main?\n");
    abort();
}