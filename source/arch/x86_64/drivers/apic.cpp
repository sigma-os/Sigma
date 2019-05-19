#include <Sigma/arch/x86_64/drivers/apic.h>

uint32_t x86_64::apic::lapic::read(uint32_t reg){
    uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_VBASE + reg);
    return *val;
}

void x86_64::apic::lapic::write(uint32_t reg, uint32_t val){
    uint32_t* data = reinterpret_cast<uint32_t*>(this->base + KERNEL_VBASE + reg);
    *data = val;
}

x86_64::apic::lapic::lapic(IPaging& paging){
    uint64_t apic_base_msr = msr::read(msr::apic_base);

    base = (apic_base_msr & 0xFFFFFFFFFFFFF000);

    bitops<uint64_t>::bit_set(apic_base_msr, 11); // Set Enable bit
    msr::write(msr::apic_base, apic_base_msr);

    paging.map_page(base, (base + KERNEL_VBASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);

    this->id = (this->read(x86_64::apic::lapic_id) >> 24) & 0xFF;

    uint32_t version_reg = this->read(x86_64::apic::lapic_version);

    this->version = (version_reg & 0xFF);
    this->max_lvt_entries = (((version_reg >> 16) & 0xFF) + 1);

    this->write(x86_64::apic::lapic_tpr, 0); // Enable all interrupts

    uint32_t spurious_reg = 0;
    spurious_reg |= 0xFF; // Hardcoded spurious vector
    bitops<uint32_t>::bit_set(spurious_reg, x86_64::apic::lapic_spurious_software_enable);
    this->write(x86_64::apic::lapic_spurious, spurious_reg);
}

void x86_64::apic::lapic::send_ipi(uint8_t target_lapic_id, uint32_t flags){
    this->write(x86_64::apic::lapic_icr_high, (target_lapic_id << 24));
    this->write(x86_64::apic::lapic_icr_low, flags);
        
    // TODO: Timeout and fail
    while((this->read(x86_64::apic::lapic_icr_low) & x86_64::apic::lapic_icr_status_pending));
}

void x86_64::apic::lapic::send_eoi(){
    this->write(x86_64::apic::lapic_eoi, 0); // Anything other than 0 *will* result in a General Protection Fault
}