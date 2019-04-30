#include <Sigma/smp/smp.h>

void smp::multiprocessing::send_ipi(uint8_t lapic_id, uint32_t flags){
    bsp_lapic->write(x86_64::apic::lapic_icr_high, (lapic_id << 24));
    bsp_lapic->write(x86_64::apic::lapic_icr_low, flags);
        
    while((bsp_lapic->read(x86_64::apic::lapic_icr_low) & x86_64::apic::lapic_icr_status_pending));
}

void smp::multiprocessing::boot_cpu(cpu_entry& e){
    debug_printf("[SMP]: Booting CPU with lapic_id: %d\n", e.lapic_id);




    this->send_ipi(e.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_levelassert | x86_64::apic::lapic_icr_dm_init));
    this->send_ipi(e.lapic_id, (x86_64::apic::lapic_icr_tm_level | x86_64::apic::lapic_icr_dm_init));    

    this->send_ipi(e.lapic_id, (x86_64::apic::lapic_icr_dm_sipi | ((0x1000 >> 12) & 0xFF)));
}

smp::multiprocessing::multiprocessing(IPaging& paging, types::linked_list<cpu_entry>& cpus, x86_64::apic::lapic* lapic){
    this->bsp_lapic = lapic;

    (void)(paging);


    uint64_t trampoline_start_addr = (uint64_t)&trampoline_start;
    uint64_t trampoline_end_addr = (uint64_t)&trampoline_end;

    memcpy(reinterpret_cast<void*>(0x1000 + KERNEL_VBASE), reinterpret_cast<void*>(trampoline_start_addr), (trampoline_end_addr - trampoline_start_addr));

    
    for(auto& e : cpus){
        if(!(e.bsp)){
            // CPU is not the BSP, boot it
            this->boot_cpu(e);
        }
    }
}