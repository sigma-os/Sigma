#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/common.h>
#include <Sigma/multiboot.h>

#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/paging.h>

#include <Sigma/arch/x86_64/misc/spinlock.h>

#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>
#include <Sigma/mm/hmm.h>

#include <Sigma/arch/x86_64/drivers/mp.h>
#include <Sigma/arch/x86_64/drivers/apic.h>

#include <Sigma/multitasking/elf.h>

#include <Sigma/smp/smp.h>
#include <Sigma/smp/cpu.h>

#include <Sigma/types/linked_list.h>

IPaging* bsp_paging = nullptr;

C_LINKAGE void kernel_main(void* multiboot_information, uint64_t magic){   
    multiboot mboot = multiboot(multiboot_information, magic);
    printf("Booting Sigma, Copyright Thomas Woertman 2019\nMemory Size: %imb\n", mboot.get_memsize_mb());

    mm::pmm::init(mboot);

    x86_64::tss::table tss = x86_64::tss::table();
    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();
    uint16_t tss_offset = gdt.add_tss(&tss);
    gdt.update_pointer();
    tss.load(tss_offset);

    x86_64::idt::idt idt = x86_64::idt::idt();
    idt.init();
    x86_64::idt::register_generic_handlers();
    
    mm::vmm::manager<x86_64::paging::paging> vmm = mm::vmm::manager<x86_64::paging::paging>();

    bsp_paging = &(vmm.get_paging_provider());

    uint64_t virt_start = KERNEL_VBASE;
    uint64_t phys_start = 0x0;

    for(uint32_t i = 0; i < (1024 * 16); i++){
        vmm.map_page(phys_start, virt_start, map_page_flags_present | map_page_flags_writable);

        virt_start += 0x1000;
        phys_start += 0x1000;
    }


    uint64_t* elf_sections_start = mboot.get_elf_sections();
    uint64_t n_elf_sections = mboot.get_elf_n_sections();

    for(uint64_t i = 0; i < n_elf_sections; i++){
        multitasking::elf::Elf64_Shdr* shdr = reinterpret_cast<multitasking::elf::Elf64_Shdr*>(reinterpret_cast<uint64_t>(elf_sections_start) + (i * sizeof(multitasking::elf::Elf64_Shdr)));
        if(shdr->sh_flags & multitasking::elf::SHF_ALLOC){
           uint32_t flags = map_page_flags_present;
           if(shdr->sh_flags & multitasking::elf::SHF_WRITE) flags |= map_page_flags_writable;
           if(!(shdr->sh_flags & multitasking::elf::SHF_EXECINSTR)) flags |= map_page_flags_no_execute;

           for(uint64_t j = 0; j < shdr->sh_size; j += mm::pmm::block_size){
                uint64_t virt = (shdr->sh_addr + j);
                uint64_t phys = (virt - KERNEL_VBASE);

                vmm.map_page(phys, virt, flags);
           }
        }
    }

    vmm.set();

    mm::hmm::init(vmm.get_paging_provider());   

    

    auto cpus = types::linked_list<smp::cpu_entry>();
    x86_64::mp::mp mp_spec = x86_64::mp::mp(cpus);
    (void)(mp_spec);

    x86_64::apic::lapic l = x86_64::apic::lapic(vmm.get_paging_provider());

    smp::multiprocessing smp = smp::multiprocessing(cpus, &l);
    (void)(smp);

    printf("Sigma: reached end of kernel_main?\n");
    abort();
}

auto ap_list = types::linked_list<smp::cpu::entry>();

x86_64::spinlock::mutex mut;

C_LINKAGE void smp_kernel_main(){
    x86_64::spinlock::acquire(&mut);


    printf("Booted CPU\n");

    auto pag = x86_64::paging::paging();
    IPaging& paging = pag;
    bsp_paging->clone_paging_info(paging);
    paging.set_paging_info();
    
    

    auto* cpu = ap_list.push_back(smp::cpu::entry());

    cpu->test = 432;


    x86_64::spinlock::release(&mut);
}