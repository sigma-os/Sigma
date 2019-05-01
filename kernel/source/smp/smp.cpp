#include <Sigma/smp/smp.h>

static bool wait_for_boot(){
    //for(volatile uint32_t i = 0; i < 100000; i++); // G

    uint64_t off = (((uint64_t)&smp::trampoline_booted) - ((uint64_t)&smp::trampoline_start));
    uint8_t* trampoline_booted_addr = (uint8_t*)((smp::smp_trampoline_base + off) + KERNEL_VBASE);

    uint64_t timeout = 100;
    while(timeout > 0){
        if(*trampoline_booted_addr == 1) return true;

        for(volatile uint32_t i = 0; i < 100000; i++); // TODO: Real timeout

        timeout--;
    }

    return false;
}

static void clear_booted_flag(){
    uint64_t off = (((uint64_t)&smp::trampoline_booted) - ((uint64_t)&smp::trampoline_start));
    uint8_t* trampoline_booted_addr = (uint8_t*)((smp::smp_trampoline_base + off) + KERNEL_VBASE);

    *trampoline_booted_addr = 0;
}

void smp::multiprocessing::boot_external_apic(smp::cpu_entry& cpu){
    x86_64::cmos::write(x86_64::cmos::reg_reset_code, x86_64::cmos::reg_reset_code_jump);

    uint32_t* reset_vector = reinterpret_cast<uint32_t*>(x86_64::bios::bios_reset_vector);
    *reset_vector = static_cast<uint32_t>(((smp::smp_trampoline_base & 0xFF00) << 12));

    this->bsp_lapic->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_levelassert | x86_64::apic::lapic_icr_dm_init));
    this->bsp_lapic->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_dm_init));  

    x86_64::cmos::write(x86_64::cmos::reg_reset_code, 0); // Reset BIOS reset
}


void smp::multiprocessing::boot_apic(smp::cpu_entry& cpu){
    this->bsp_lapic->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_levelassert | x86_64::apic::lapic_icr_dm_init));
    //this->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_dm_init));    

    this->bsp_lapic->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_dm_sipi | ((smp::smp_trampoline_base >> 12) & 0xFF)));
    if(!wait_for_boot()) this->bsp_lapic->send_ipi(cpu.lapic_id, (x86_64::apic::lapic_icr_dm_sipi | ((smp::smp_trampoline_base >> 12) & 0xFF)));
}

void smp::multiprocessing::boot_cpu(cpu_entry& e){
    
    uint64_t off = (((uint64_t)&smp::trampoline_stack) - ((uint64_t)&smp::trampoline_start));
    uint64_t* trampoline_stack_addr = (uint64_t*)((smp::smp_trampoline_base + off) + KERNEL_VBASE);

    void* cpu_stack = mm::pmm::alloc_block();

    *trampoline_stack_addr = reinterpret_cast<uint64_t>(cpu_stack);

    if(e.lapic_version >= 0x10){
        // boot normal apic

        this->boot_apic(e);
    } else {
        // boot external apic. on 64bit cpu?

        this->boot_external_apic(e);
    }

    if(wait_for_boot()){
        debug_printf("[SMP]: Booted CPU with lapic_id: %d, stack: %x\n", e.lapic_id, *trampoline_stack_addr);
    } else {
        debug_printf("[SMP]: Failed to boot CPU with lapic_id: %d\n", e.lapic_id);
        mm::pmm::free_block(cpu_stack);
    }

    
    clear_booted_flag(); // Clear flag for next CPU
}

smp::multiprocessing::multiprocessing(IPaging& paging, types::linked_list<cpu_entry>& cpus, x86_64::apic::lapic* lapic){
    this->bsp_lapic = lapic;

    (void)(paging);


    uint64_t trampoline_start_addr = (uint64_t)&trampoline_start;
    uint64_t trampoline_end_addr = (uint64_t)&trampoline_end;

    memcpy(reinterpret_cast<void*>(smp::smp_trampoline_base + KERNEL_VBASE), reinterpret_cast<void*>(trampoline_start_addr), (trampoline_end_addr - trampoline_start_addr));

    
    for(auto& e : cpus){
        if(!(e.bsp)){
            // CPU is not the BSP, boot it
            this->boot_cpu(e);
        }
    }
}