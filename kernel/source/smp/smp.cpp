#include <Sigma/smp/smp.h>

static bool wait_for_boot(){
    uint8_t* trampoline_booted_addr = &smp::trampoline_booted;

    uint64_t timeout = 100;
    while(timeout > 0){
        if(*trampoline_booted_addr == 1) return true;

        for(size_t i = 0; i < 100000; i++) asm("nop"); // TODO: Real timeout

        timeout--;
    }

    return false;
}

static void clear_booted_flag(){
    uint8_t* trampoline_booted_addr = &smp::trampoline_booted;
    *trampoline_booted_addr = 0;
}

void smp::multiprocessing::boot_apic(smp::cpu_entry& cpu){
    auto& lapic = smp::cpu::get_current_cpu()->lapic;
    lapic.send_ipi_raw(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_levelassert | x86_64::apic::lapic_icr_dm_init));
    //this->send_ipi_raw(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_dm_init));    
    lapic.send_ipi_raw(cpu.lapic_id, (x86_64::apic::lapic_icr_dm_sipi | ((smp::smp_trampoline_base >> 12) & 0xFF)));
    if(!wait_for_boot()) lapic.send_ipi_raw(cpu.lapic_id, (x86_64::apic::lapic_icr_dm_sipi | ((smp::smp_trampoline_base >> 12) & 0xFF)));
}

C_LINKAGE uint64_t trampoline_stack;
C_LINKAGE uint64_t trampoline_paging;
C_LINKAGE uint8_t trampoline_booted;

void smp::multiprocessing::boot_cpu(cpu_entry& e){
    constexpr size_t ap_stack_size = 4;

    uint64_t* trampoline_stack_addr = &smp::trampoline_stack;
    void* cpu_stack = mm::pmm::alloc_n_blocks(ap_stack_size); // 16kb stack
    if(cpu_stack == nullptr){
        debug_printf("Failed to allocate stack for CPU with lapic_id: %d\n", e.lapic_id);
        return;
    }
    
    *trampoline_stack_addr = (reinterpret_cast<uint64_t>(cpu_stack) + (mm::pmm::block_size * ap_stack_size) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    
    uint64_t* trampoline_paging_addr = &smp::trampoline_paging;
    *trampoline_paging_addr = (mm::vmm::kernel_vmm::get_instance().get_paging_info() - KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

    this->boot_apic(e);

    if(wait_for_boot()){
        debug_printf("[SMP]: Booted CPU with lapic_id: %d, stack: %x\n", e.lapic_id, *trampoline_stack_addr);
    } else {
        debug_printf("[SMP]: Failed to boot CPU with lapic_id: %d\n     TODO: Implement multiframe freeing\n", e.lapic_id);
    }

    clear_booted_flag(); // Clear flag for next CPU
}

smp::multiprocessing::multiprocessing(types::linked_list<cpu_entry>& cpus): cpus{cpus} {
    uint64_t trampoline_start_addr = (uint64_t)&trampoline_start;
    uint64_t trampoline_end_addr = (uint64_t)&trampoline_end;

    for(uint64_t i = 0; i < (trampoline_end_addr - trampoline_start_addr); i += mm::pmm::block_size)
        mm::vmm::kernel_vmm::get_instance().map_page(smp::smp_trampoline_base + i, (smp::smp_trampoline_base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + i), map_page_flags_present | map_page_flags_writable);    

    memcpy(reinterpret_cast<void*>(smp::smp_trampoline_base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), reinterpret_cast<void*>(trampoline_start_addr), (trampoline_end_addr - trampoline_start_addr));
    
    smp::ipi::init_ipi();
}

void smp::multiprocessing::boot_aps(){
    for(auto& e : cpus)
        if(!(e.bsp)) // CPU is not the BSP, boot it
            this->boot_cpu(e);
}