#include <Sigma/common.h>

#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/idt.h>

#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/multiboot.h>

C_LINKAGE void kernel_main(void* multiboot_information, uint64_t magic){   
    multiboot mboot = multiboot(multiboot_information, magic);
    (void)(mboot);


    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();

    x86_64::idt::idt idt = x86_64::idt::idt();
    idt.init();


    printf("Sigma: reached end of kernel_main?\n");
    abort();
}