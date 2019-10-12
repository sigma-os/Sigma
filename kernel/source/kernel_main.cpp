#include <klibc/stdio.h>
#include <klibc/stdlib.h>

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/paging.h>

#include <Sigma/arch/x86_64/misc/spinlock.h>

#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>
#include <Sigma/mm/hmm.h>

#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/drivers/hpet.h>
#include <Sigma/arch/x86_64/drivers/pic.h>
#include <Sigma/arch/x86_64/drivers/pci.h>
#include <Sigma/arch/x86_64/misc/misc.h>

#include <Sigma/smp/smp.h>
#include <Sigma/smp/cpu.h>

#include <Sigma/acpi/acpi.h>
#include <Sigma/acpi/madt.h>

#include <Sigma/proc/initrd.h>
#include <Sigma/proc/process.h>
#include <Sigma/proc/syscall.h>
#include <Sigma/proc/elf.h>

#include <Sigma/types/linked_list.h>
#include <Sigma/boot_protocol.h>
#include <config.h>

auto cpu_list = misc::lazy_initializer<types::vector<smp::cpu::entry>>();
C_LINKAGE boot::boot_protocol boot_data;

static void enable_cpu_tasking(){
    uint64_t rsp = 0;
    asm("mov %%rsp, %0" : "=r"(rsp)); 
    smp::cpu::get_current_cpu()->tss->rsp0 = rsp;

    proc::process::init_cpu();

    asm("sti"); // Start interrupts and wait for an APIC timer IRQ to arrive for the first scheduling task
    while(1);
}

C_LINKAGE void kernel_main(){  
    FUNCTION_CALL_ONCE();

    auto* boot_protocol = &boot_data;
    printf("Booting Sigma %s, Copyright Thomas Woertman 2019\nMemory Size: %imb\n", VERSION_STR, boot_protocol->memsize);

    misc::kernel_args::init(boot_protocol->cmdline);
    
    mm::pmm::init(boot_protocol);

    x86_64::tss::table tss = x86_64::tss::table();
    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();
    uint16_t tss_offset = gdt.add_tss(&tss);
    tss.load(tss_offset);

    x86_64::idt::idt idt = x86_64::idt::idt();
    idt.init();
    x86_64::idt::register_generic_handlers();    

    uint64_t virt_start = KERNEL_VBASE;
    uint64_t phys_start = 0x0;
    
    for(uint32_t i = 0; i < (1024 * 16); i++){
        mm::vmm::kernel_vmm::get_instance().map_page(phys_start, virt_start, map_page_flags_present | map_page_flags_writable | map_page_flags_global);

        virt_start += 0x1000;
        phys_start += 0x1000;
    }


    uint64_t* elf_sections_start = reinterpret_cast<uint64_t*>(boot_protocol->kernel_elf_sections);
    uint64_t n_elf_sections = boot_protocol->kernel_n_elf_sections;

    for(uint64_t i = 0; i < n_elf_sections; i++){
        proc::elf::Elf64_Shdr* shdr = reinterpret_cast<proc::elf::Elf64_Shdr*>(reinterpret_cast<uint64_t>(elf_sections_start) + (i * sizeof(proc::elf::Elf64_Shdr)));
        if(shdr->sh_flags & proc::elf::shf_alloc){
           uint32_t flags = map_page_flags_present | map_page_flags_global;
           if(shdr->sh_flags & proc::elf::shf_write) flags |= map_page_flags_writable;
           if(!(shdr->sh_flags & proc::elf::shf_execinstr)) flags |= map_page_flags_no_execute;

           for(uint64_t j = 0; j < shdr->sh_size; j += mm::pmm::block_size){
                uint64_t virt = (shdr->sh_addr + j);
                uint64_t phys = (virt - KERNEL_VBASE);

                mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, flags);
           }
        }
    }

    mm::vmm::kernel_vmm::get_instance().set();

    mm::hmm::init(); 


    auto cpus = types::linked_list<smp::cpu_entry>();

    acpi::init(boot_protocol);
    acpi::madt madt = acpi::madt();

    if(madt.found_table()){
        madt.parse();

        madt.get_cpus(cpus);

        if(madt.supports_legacy_pic()){
            x86_64::pic::set_base_vector(32);
            x86_64::pic::disable(); 
        }
    } else {
        PANIC("Didn't find MADT table\n Can't continue boot");
    }

    x86_64::apic::lapic lapic = x86_64::apic::lapic();
    lapic.init(); 

    cpu_list.init();

    auto* entry = cpu_list->empty_entry();;
    entry->lapic = lapic;
    entry->lapic_id = entry->lapic.get_id();
    entry->tss = &tss;
    entry->set_gs();


    x86_64::idt::register_irq_status(33, true);

    x86_64::apic::ioapic::init(madt);


    x86_64::pci::parse_pci();

    x86_64::hpet::init_hpet();

    acpi::init_sci(madt);

    proc::process::init_multitasking(madt);

    smp::multiprocessing smp = smp::multiprocessing(cpus, &lapic);
    (void)(smp);

    for(uint64_t i = 0; i < boot_data.kernel_initrd_size; i += mm::pmm::block_size){
        mm::vmm::kernel_vmm::get_instance().map_page(boot_data.kernel_initrd_ptr + i, (boot_data.kernel_initrd_ptr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + i), map_page_flags_present | map_page_flags_global | map_page_flags_no_execute);    
    }

    proc::initrd::init((boot_data.kernel_initrd_ptr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), boot_data.kernel_initrd_size);

    proc::syscall::init_syscall();

    // TODO: Start this in modular way
    proc::process::thread* thread = nullptr;
    if(!proc::elf::start_elf_executable("/usr/bin/zeta", &thread)) printf("Failed to load Zeta\n");
    proc::syscall::set_user_manager_tid(thread->tid);

    /*proc::process::create_kernel_thread(+[](){
        // TODO: Initialize ACPI kernel thread and PCI kernel thread


        proc::process::set_thread_state(proc::process::get_current_thread(), proc::process::thread_state::BLOCKED);
    });*/

    enable_cpu_tasking();
    asm("cli; hlt"); // Wait what?
}   



x86_64::spinlock::mutex ap_mutex = x86_64::spinlock::mutex();

C_LINKAGE void smp_kernel_main(){
    ap_mutex.acquire();

    x86_64::tss::table tss = x86_64::tss::table();
    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();
    uint16_t tss_offset = gdt.add_tss(&tss);
    gdt.update_pointer();
    tss.load(tss_offset);

    x86_64::idt::idt idt = x86_64::idt::idt();
    idt.init();

    auto* entry = cpu_list->empty_entry();

    entry->lapic = x86_64::apic::lapic();
    entry->lapic.init();
    entry->lapic_id = entry->lapic.get_id();
    entry->tss = &tss;
    entry->set_gs();

    printf("Booted CPU with lapic_id: %d\n", entry->lapic_id);

    ap_mutex.release();

    enable_cpu_tasking();
    asm("cli; hlt"); // Wait what?
}
