#include <Sigma/common.h>

#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/idt.h>

#include <Sigma/arch/x86_64/drivers/mp.h>

#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>

#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/multiboot.h>

#include <Sigma/arch/x86_64/paging.h>



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

    mm::pmm::init(mboot);
    
    mm::vmm::manager<x86_64::paging::paging> vmm = mm::vmm::manager<x86_64::paging::paging>();

    uint64_t virt_start = KERNEL_VBASE;
    uint64_t phys_start = 0x0;

    for(uint32_t i = 0; i < (1024 * 16); i++){
        vmm.map_page(phys_start, virt_start, map_page_flags_present | map_page_flags_writable);

        virt_start += 0x1000;
        phys_start += 0x1000;
    }

    vmm.set();

    x86_64::mp::mp mp_spec = x86_64::mp::mp();
    (void)(mp_spec);

    printf("Sigma: reached end of kernel_main?\n");
    abort();
}