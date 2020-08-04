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
#include <Sigma/arch/x86_64/cpu.h>

#include <Sigma/smp/smp.h>
#include <Sigma/smp/cpu.h>

#include <Sigma/acpi/acpi.h>
#include <Sigma/acpi/madt.h>

#include <Sigma/proc/initrd.h>
#include <Sigma/proc/process.h>
#include <Sigma/proc/syscall.h>
#include <Sigma/proc/elf.h>

#include <Sigma/generic/device.h>
#include <Sigma/generic/virt.hpp>

#include <Sigma/types/linked_list.h>
#include <Sigma/types/minimal_array.h>
#include <Sigma/boot_protocol.h>
#include <config.h>
#include <cxxabi.h>
#include <Sigma/arch/x86_64/intel/vt-d.hpp>
#include <Sigma/generic/user_handle.hpp>

types::minimal_array<1, smp::cpu::entry> cpu_list{};
C_LINKAGE boot::boot_protocol boot_data;

static void enable_cpu_tasking(){
    uint64_t rsp = (uint64_t)smp::cpu::get_current_cpu()->kstack->top();
    rsp = ALIGN_DOWN(rsp, 16); // Align stack to ABI requirements
    smp::cpu::get_current_cpu()->tss.rsp0 = rsp;

    proc::process::init_cpu();

    asm("sti"); // Start interrupts and wait for an APIC timer IRQ to arrive for the first scheduling task
    while(1)
        ;
}

C_LINKAGE void kernel_main(){  
    FUNCTION_CALL_ONCE();

    auto* boot_protocol = &boot_data;
    printf("Booting Sigma %s, Copyright Thomas Woertman 2019\n", VERSION_STR);

    misc::kernel_args::init(boot_protocol->cmdline);
    proc::elf::init_symbol_list(*boot_protocol);
    mm::pmm::init(boot_protocol);

    auto& entry = cpu_list.empty_entry();
    entry.set_gs();
    entry.pcid_context = {};
    entry.gdt = {};
    entry.tss = {};
    entry.gdt.init();
    entry.tss.init();
    entry.tss_gdt_offset = entry.gdt.add_tss(entry.tss);
    entry.tss.load(entry.tss_gdt_offset);

    x86_64::misc_early_features_init();

    x86_64::idt::init();
    x86_64::idt::register_generic_handlers();    

    x86_64::idt::load();


    auto& vmm = mm::vmm::kernel_vmm::get_instance();

    uint64_t virt_start = KERNEL_VBASE;
    uint64_t phys_start = 0x0;

    for(size_t i = 0; i < (1024 * 20); i++){
        vmm.map_page(phys_start, virt_start, map_page_flags_present | map_page_flags_writable | map_page_flags_global | map_page_flags_no_execute);

        virt_start += 0x1000;
        phys_start += 0x1000;
    }

    {
        auto* mmap = reinterpret_cast<multiboot_tag_mmap*>(boot_data.mmap);
        for(multiboot_memory_map_t* mmap_entry = mmap->entries; (uintptr_t)mmap_entry < ((uintptr_t)mmap + mmap->size); mmap_entry++){
            if(mmap_entry->type == MULTIBOOT_MEMORY_AVAILABLE){
                auto bottom = ALIGN_DOWN(mmap_entry->addr, mm::pmm::block_size);
                auto top = ALIGN_UP(mmap_entry->addr + mmap_entry->len, mm::pmm::block_size);
                
                for(; bottom < top; bottom += mm::pmm::block_size){
                    vmm.map_page(bottom, bottom + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present | map_page_flags_writable | map_page_flags_global | map_page_flags_no_execute);
                }
            }

        }
    }

    proc::elf::map_kernel(*boot_protocol);
    vmm.set();

    mm::hmm::init(); 

    entry.idle_stack.init();
    entry.kstack.init();    

    // Initialize initrd as early as possible so it can be used for reading files for command line args
    for(uint64_t i = 0; i < boot_data.kernel_initrd_size; i += mm::pmm::block_size){
        vmm.map_page(boot_data.kernel_initrd_ptr + i, (boot_data.kernel_initrd_ptr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + i), map_page_flags_present | map_page_flags_global | map_page_flags_no_execute);    
    }

    proc::initrd::init((boot_data.kernel_initrd_ptr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), boot_data.kernel_initrd_size);

    generic::device::init();

    acpi::init();
    acpi::madt madt{};

    if(madt.found_table()){
        madt.parse();

        if(madt.supports_legacy_pic()){
            x86_64::pic::set_base_vector(32);
            x86_64::pic::disable(); 
        }
    } else {
        PANIC("Didn't find MADT table\nCan't continue boot");
    }

    entry.lapic = {};
    entry.lapic.init();
    entry.lapic_id = entry.lapic.get_id();

    x86_64::identify_cpu();
    x86_64::idt::register_irq_status(33, true);
    x86_64::apic::ioapic::init(madt);
    x86_64::pci::init_pci();
    x86_64::hpet::init_hpet();
    acpi::init_sci(madt);
    x86_64::pci::parse_pci();

    proc::process::init_multitasking(madt);
    proc::syscall::init_syscall();
    smp::multiprocessing smp{madt.get_cpus()};
    smp.boot_aps();

    x86_64::misc_bsp_late_features_init();


    proc::process::make_kernel_thread(proc::process::create_blocked_thread(proc::process::thread_privilege_level::KERNEL), [](){
        constexpr size_t interval = 50;
        debug_printf("[ALLOC]: Running heap corruption watchdog on TID: %x, checking for heap corruption every %d ms...\n", proc::process::get_current_tid(), interval);

        while(true) {
            if(alloc::check_for_corruption(false))
                alloc::check_for_corruption(true); // First check if its corrupted, then dump all the info

            x86_64::hpet::poll_sleep(interval);
        }
    });

    //proc::process::thread* kbus = nullptr;
    //if(!proc::elf::start_elf_executable("/usr/bin/kbus", &kbus, proc::process::thread_privilege_level::DRIVER)) printf("Failed to load kbus\n");

    //proc::process::thread* vfs = nullptr;
    //if(!proc::elf::start_elf_executable("/usr/bin/zeta", &vfs, proc::process::thread_privilege_level::DRIVER)) printf("Failed to load Zeta\n");

    // TODO: Start this in modular way
    proc::process::thread* block = nullptr;
    if(!proc::elf::start_elf_executable("/usr/bin/nvme", &block, proc::process::thread_privilege_level::DRIVER)) printf("Failed to load nvme\n");

    enable_cpu_tasking();
    asm("cli; hlt"); // Wait what?
}   



C_LINKAGE void smp_kernel_main(){
    /*x86_64::tss::table tss = x86_64::tss::table();
    x86_64::gdt::gdt gdt = x86_64::gdt::gdt();
    gdt.init();
    uint16_t tss_offset = gdt.add_tss(&tss);
    gdt.update_pointer();
    tss.load(tss_offset);*/

    auto& entry = cpu_list.empty_entry();
    entry.set_gs();

    entry.gdt = {};
    entry.tss = {};
    entry.gdt.init();
    entry.tss.init();
    entry.tss_gdt_offset = entry.gdt.add_tss(entry.tss);
    entry.tss.load(entry.tss_gdt_offset);

    x86_64::idt::load();
    
    entry.lapic = {};
    entry.lapic.init();
    entry.lapic_id = entry.lapic.get_id();
    entry.pcid_context = x86_64::paging::pcid_cpu_context{};
    entry.idle_stack.init();
    entry.kstack.init();

    x86_64::misc_early_features_init();

    mm::vmm::kernel_vmm::get_instance().set();

    //printf("Booted CPU with lapic_id: %d\n", entry.lapic_id);

    enable_cpu_tasking();
    asm("cli; hlt"); // Wait what?
}
