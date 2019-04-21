#include <Sigma/common.h>

#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/idt.h>

#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/multiboot.h>

C_LINKAGE void kernel_main(void* multiboot_information, uint64_t magic){   
    multiboot mboot = multiboot(multiboot_information, magic);
    printf("Booting Sigma, Copyright Thomas Woertman 2019\nMemory Size: %imb\n", mboot.get_memsize_mb());

    x86_64::tss::table tss = x86_64::tss::table();
    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();
    uint16_t tss_offset = gdt.add_tss(&tss);
    gdt.update_pointer();
    tss.load(tss_offset);

    x86_64::idt::idt idt = x86_64::idt::idt();
    idt.init();

    printf("Sigma: reached end of kernel_main?\n");
    abort();
}