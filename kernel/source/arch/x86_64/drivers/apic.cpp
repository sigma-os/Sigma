#include <Sigma/acpi/madt.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/drivers/hpet.h>

#pragma region LAPIC

uint32_t x86_64::apic::lapic::read(uint32_t reg){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + reg);
    return *val;
}

void x86_64::apic::lapic::write(uint32_t reg, uint32_t data){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + reg);
    *val = data;
}

void x86_64::apic::lapic::init(){
    uint64_t apic_base_msr = msr::read(msr::apic_base);

    base = (apic_base_msr & 0xFFFFFFFFFFFFF000);

    bitops<uint64_t>::bit_set(apic_base_msr, 11); // Set Enable bit
    msr::write(msr::apic_base, apic_base_msr);

    mm::vmm::kernel_vmm::get_instance().map_page(base, (base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);

    this->id = (this->read(x86_64::apic::lapic_id) >> 24) & 0xFF;

    uint32_t version_reg = this->read(x86_64::apic::lapic_version);

    this->version = (version_reg & 0xFF);
    this->max_lvt_entries = (((version_reg >> 16) & 0xFF) + 1);

    this->write(x86_64::apic::lapic_tpr, 0); // Enable all interrupts
    this->write(x86_64::apic::lapic_ldr, (this->get_id() << 16));
    this->write(x86_64::apic::lapic_dfr, 0xFFFFFFFF);

    uint32_t spurious_reg = 0;
    spurious_reg |= 0xFF; // Hardcoded spurious vector
    bitops<uint32_t>::bit_set(spurious_reg, x86_64::apic::lapic_spurious_software_enable);
    this->write(x86_64::apic::lapic_spurious, spurious_reg);
}

void x86_64::apic::lapic::send_ipi_raw(uint8_t target_lapic_id, uint32_t flags){
    this->write(x86_64::apic::lapic_icr_high, (target_lapic_id << 24));
    this->write(x86_64::apic::lapic_icr_low, flags);
        
    // TODO: Timeout and fail
    while((this->read(x86_64::apic::lapic_icr_low) & x86_64::apic::lapic_icr_status_pending));
}

void x86_64::apic::lapic::send_ipi(uint8_t target_lapic_id, uint8_t vector){
    this->write(x86_64::apic::lapic_icr_high, (target_lapic_id << 24));
    this->write(x86_64::apic::lapic_icr_low, vector);
        
    // TODO: Timeout and fail
    while((this->read(x86_64::apic::lapic_icr_low) & x86_64::apic::lapic_icr_status_pending));
}

void x86_64::apic::lapic::send_eoi(){
    this->write(x86_64::apic::lapic_eoi, 0); // Anything other than 0 *will* result in a General Protection Fault
}

void x86_64::apic::lapic::enable_timer(uint8_t vector, uint64_t ms, x86_64::apic::lapic_timer_modes mode){  
    uint32_t ticks_per_second;

    if(this->timer_ticks_per_ms == 0){ // Speed is unknown so calibrate
        this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16
        this->write(x86_64::apic::lapic_timer_initial_count, 0xFFFFFFFF);
        this->set_timer_mask(false);

        x86_64::hpet::poll_sleep(10);

        this->set_timer_mask(true);

        ticks_per_second = 0xFFFFFFFF - this->read(x86_64::apic::lapic_timer_current_count);
        this->timer_ticks_per_ms = ticks_per_second / 10;
    }

    this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16
    this->set_timer_mode(mode);
    this->set_timer_vector(vector);
    this->write(x86_64::apic::lapic_timer_initial_count, this->timer_ticks_per_ms * ms);

    this->set_timer_mask(false);
}

void x86_64::apic::lapic::set_timer_mode(x86_64::apic::lapic_timer_modes mode){
    uint32_t lint_entry = this->read(x86_64::apic::lapic_lvt_timer);

    lint_entry &= ~(0b11 << 17); // Clear Mode bits
    lint_entry |= (misc::as_integer(mode) << 17);

    this->write(x86_64::apic::lapic_lvt_timer, lint_entry);
}

void x86_64::apic::lapic::set_timer_vector(uint8_t vector){
    this->write(x86_64::apic::lapic_lvt_timer, ((this->read(x86_64::apic::lapic_lvt_timer) & 0xFFFFFF00) | vector));
}

void x86_64::apic::lapic::set_timer_mask(bool state){
    uint32_t lint_entry = this->read(x86_64::apic::lapic_lvt_timer);

    if(state) bitops<uint32_t>::bit_set(lint_entry, x86_64::apic::lapic_lvt_mask);
    else bitops<uint32_t>::bit_clear(lint_entry, x86_64::apic::lapic_lvt_mask);

    this->write(x86_64::apic::lapic_lvt_timer, lint_entry);
}

#pragma endregion

#pragma region IOAPIC

static inline uint32_t get_redirection_entry(uint32_t entry){
    return (x86_64::apic::ioapic_redirection_table + entry * 2);
}

void x86_64::apic::ioapic_device::init(uint64_t base){
    this->base = base;

    mm::vmm::kernel_vmm::get_instance().map_page(base, (base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);

    this->id = ((this->read(x86_64::apic::ioapic_id) >> 24) & 0xF);
    uint32_t ver = this->read(x86_64::apic::ioapic_ver);
    this->version = (ver & 0xFF);
    this->max_redirection_entries = ((ver >> 16) & 0xFF) + 1;

    for(size_t i = 0; i < this->max_redirection_entries; i++){
        this->set_entry(i, (0 | (1 << 16))); // Mask all unused interrupts
    }

    debug_printf("[I/OAPIC]: Initialized I/OAPIC: ID: %x, Version: %x, n_redirection entries: %d\n", this->id, this->version, this->max_redirection_entries);
}

uint32_t x86_64::apic::ioapic_device::read(uint32_t reg){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + x86_64::apic::ioapic_register_select);
    *val = reg;
    volatile uint32_t* dat = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + x86_64::apic::ioapic_register_data);
    return *dat;
}

void x86_64::apic::ioapic_device::write(uint32_t reg, uint32_t data){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + x86_64::apic::ioapic_register_select);
    *val = reg;
    volatile uint32_t* dat = reinterpret_cast<uint32_t*>(this->base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE + x86_64::apic::ioapic_register_data);
    *dat = data;
}
void x86_64::apic::ioapic_device::unmask(uint8_t index){
    this->set_entry(index, (this->read_entry(index) & ~(1 << 16)));
}

uint64_t x86_64::apic::ioapic_device::read_entry(uint8_t index){
    uint8_t offset = get_redirection_entry(index);
    uint32_t low = this->read(offset);
    uint32_t high = this->read(offset + 1);
    return (low | ((uint64_t)high << 32));
}

void x86_64::apic::ioapic_device::set_entry(uint8_t index, uint64_t data){
    uint8_t offset = get_redirection_entry(index);
    this->write(offset, (data & 0xFFFFFFFF));
    this->write(offset + 1, ((data >> 32) & 0xFFFFFFFF));
}

void x86_64::apic::ioapic_device::set_entry(uint8_t index, uint8_t vector, x86_64::apic::ioapic_delivery_modes delivery_mode, x86_64::apic::ioapic_destination_modes destination_mode, uint16_t flags, uint8_t destination){
    uint64_t data = 0;
    data |= vector;
    data |= (misc::as_integer(delivery_mode) << 8);
    data |= (misc::as_integer(destination_mode) << 11);
    
    if(flags & 2) data |= ((uint64_t)1 << 13);
    if(flags & 8) data |= ((uint64_t)1 << 15);
    
    data |= ((uint64_t)destination << 56);
    this->set_entry(index, data);
}

auto interrupt_overrides = types::linked_list<x86_64::apic::interrupt_override>();
auto ioapics = types::linked_list<std::pair<x86_64::apic::ioapic_device, uint32_t>>(); // ioapic, and gsi_base

void x86_64::apic::ioapic::init(acpi::madt& madt){
    auto ioapics_base = types::linked_list<std::pair<uint64_t, uint32_t>>(); // base, and gsi_base
    madt.get_ioapics(ioapics_base);
    madt.get_interrupt_overrides(interrupt_overrides);

    for(auto& a : ioapics_base){
        auto* ioapic = ioapics.empty_entry();
        ioapic->first.init(a.first);
        ioapic->second = a.second;
    }

    if(madt.supports_legacy_pic()){
        // First 16 GSI's are mapped to PIC interrupts, with ISO exceptions
        for(size_t i = 0; i < 16; i++){
            if(i == 2) continue; // Skip mapping Slave Cascade

            bool found = false;
            for(auto& entry : interrupt_overrides){
                if(entry.source == i){
                    // Found ISO for this interrupt

                    x86_64::apic::ioapic::set_entry(entry.gsi, (entry.source + 0x20), x86_64::apic::ioapic_delivery_modes::FIXED, x86_64::apic::ioapic_destination_modes::PHYSICAL, entry.flags, smp::cpu::get_current_cpu()->lapic_id); // Target all LAPIC's
                    //x86_64::apic::ioapic::set_entry(entry.gsi, (x86_64::apic::ioapic::read_entry(entry.gsi) | (1 << 16))); // Mask all entries
                    x86_64::apic::ioapic::mask_gsi(entry.gsi);
                    found = true;
                    break;
                }
            }
            if(!found){
                // Assume GSI = IRQ
                x86_64::apic::ioapic::set_entry(i, (i + 0x20), x86_64::apic::ioapic_delivery_modes::FIXED, x86_64::apic::ioapic_destination_modes::PHYSICAL, 0, smp::cpu::get_current_cpu()->lapic_id); // Target all LAPIC's
                //x86_64::apic::ioapic::set_entry(i, (x86_64::apic::ioapic::read_entry(i) | (1 << 16))); // Mask all entries
                x86_64::apic::ioapic::mask_gsi(i);
            }
            x86_64::idt::register_irq_status((i + 0x20), true); // Set IRQ status to true
        }
    }
}

void x86_64::apic::ioapic::set_entry(uint32_t gsi, uint8_t vector, ioapic_delivery_modes delivery_mode, ioapic_destination_modes destination_mode, uint16_t flags, uint8_t destination){
    for(auto& entry : ioapics){
        if((entry.second <= gsi) && ((entry.first.get_max_redirection_entries() + entry.second) > gsi)){
            // Found correct ioapic
            entry.first.set_entry((gsi - entry.second), vector, delivery_mode, destination_mode, flags, destination);
            return;
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC writing gsi entry: %x\n", gsi);
}

void x86_64::apic::ioapic::set_entry(uint32_t gsi, uint64_t data){
    for(auto& entry : ioapics){
        if((entry.second <= gsi) && ((entry.first.get_max_redirection_entries() + entry.second) > gsi)){
            // Found correct ioapic
            entry.first.set_entry(gsi - entry.second, data);
            return;
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC writing gsi entry: %x\n", gsi);
}

uint64_t x86_64::apic::ioapic::read_entry(uint32_t gsi){
    for(auto& entry : ioapics){
        if((entry.second <= gsi) && ((entry.first.get_max_redirection_entries() + entry.second) > gsi)){
            // Found correct ioapic
            return entry.first.read_entry(gsi - entry.second);
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC for reading gsi entry: %x\n", gsi);
    return 0;
}

void x86_64::apic::ioapic::mask_gsi(uint32_t gsi){
    for(auto& entry : ioapics){
        if((entry.second <= gsi) && ((entry.first.get_max_redirection_entries() + entry.second) > gsi)){
            // Found correct ioapic
            entry.first.set_entry((gsi - entry.second), (entry.first.read_entry((gsi - entry.second)) | (1 << 16))); // Mask
            return;
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC for masking gsi: %x\n", gsi);
}

void x86_64::apic::ioapic::unmask_gsi(uint32_t gsi){
    for(auto& entry : ioapics){
        if((entry.second <= gsi) && ((entry.first.get_max_redirection_entries() + entry.second) > gsi)){
            // Found correct ioapic
            entry.first.unmask((gsi - entry.second));
            return;
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC for unmasking gsi: %x\n", gsi);
}

void x86_64::apic::ioapic::unmask_irq(uint32_t irq){
    for(const auto& iso : interrupt_overrides){
        if(iso.source == irq){
            x86_64::apic::ioapic::unmask_gsi(iso.gsi);
            return;
        }
    }

    x86_64::apic::ioapic::unmask_gsi(irq);
}

#pragma endregion