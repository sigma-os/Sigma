#include <Sigma/acpi/madt.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/drivers/hpet.h>

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

    if(this->timer_ticks_per_ms != 0) goto finalize;

    this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16


    this->write(x86_64::apic::lapic_timer_initial_count, 0xFFFFFFFF);
    this->set_timer_mask(false);

    x86_64::hpet::poll_sleep(10);

    this->set_timer_mask(true);

    ticks_per_second = 0xFFFFFFFF - this->read(x86_64::apic::lapic_timer_current_count);
    this->timer_ticks_per_ms = ticks_per_second / 10;

finalize:
    this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16
    this->set_timer_mode(mode);
    this->set_timer_vector(vector);
    this->write(x86_64::apic::lapic_timer_initial_count, this->timer_ticks_per_ms * ms);

    this->set_timer_mask(false);
}

void x86_64::apic::lapic::set_timer_mode(x86_64::apic::lapic_timer_modes mode){
    uint32_t lint_entry = this->read(x86_64::apic::lapic_lvt_timer);
    switch (mode)
    {
    case x86_64::apic::lapic_timer_modes::ONE_SHOT:
        bitops<uint32_t>::bit_clear(lint_entry, 17);
        bitops<uint32_t>::bit_clear(lint_entry, 18);
        break;

    case x86_64::apic::lapic_timer_modes::PERIODIC:
        bitops<uint32_t>::bit_set(lint_entry, 17);
        bitops<uint32_t>::bit_clear(lint_entry, 18);
        break;
        
    case x86_64::apic::lapic_timer_modes::TSC_DEADLINE:
        bitops<uint32_t>::bit_clear(lint_entry, 17);
        bitops<uint32_t>::bit_set(lint_entry, 18);
        break;
    
    default:
        break;
    }

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

// IOAPIC

static inline uint32_t get_redirection_entry(uint32_t entry){
    return (x86_64::apic::ioapic_redirection_table + entry * 2);
}

void x86_64::apic::ioapic_device::init(uint64_t base, uint32_t gsi_base, bool pic, types::linked_list<x86_64::apic::interrupt_override>& isos){
    this->base = base;

    mm::vmm::kernel_vmm::get_instance().map_page(base, (base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);

    this->id = ((this->read(x86_64::apic::ioapic_id) >> 24) & 0xF);
    uint32_t ver = this->read(x86_64::apic::ioapic_ver);
    this->version = (ver & 0xFF);
    this->max_redirection_entries = ((ver >> 16) & 0xFF) + 1;

    for(size_t i = 0; i < this->max_redirection_entries; i++){
        this->set_entry(i, (0 | (1 << 16))); // Mask all unused interrupts
    }

    if(pic && gsi_base == 0){ // TODO: Don't assume that this IOAPIC has all interrupt entries
        // First 16 GSI's are mapped to PIC interrupts, with ISO exceptions
        for(size_t i = 0; i < 16; i++){
            if(i == 2) continue; // Skip mapping Slave Cascade

            bool found = false;
            for(auto& entry : isos){
                if(entry.source == i){
                    // Found ISO for this interrupt
                    this->set_entry(entry.gsi, (entry.source + 0x20), x86_64::apic::ioapic_delivery_modes::LOW_PRIORITY, x86_64::apic::ioapic_destination_modes::LOGICAL, ((entry.polarity == 3) ? (1) : (0)), ((entry.trigger_mode == 3) ? (1) : (0)), 0xFF); // Target all LAPIC's
                    this->set_entry(entry.gsi, (this->read_entry(entry.gsi) | (1 << 16))); // Mask all entries
                    found = true;
                    break;
                }
            }
            if(!found){
                // Assume GSI = IRQ
                this->set_entry(i, (i + 0x20), x86_64::apic::ioapic_delivery_modes::LOW_PRIORITY, x86_64::apic::ioapic_destination_modes::LOGICAL, 0, 0, 0xFF); // Target all LAPIC's
                this->set_entry(i, (this->read_entry(i) | (1 << 16))); // Mask all entries

            }
            x86_64::idt::register_irq_status(i, true); // Set IRQ status to true
        }
    }

    debug_printf("[IOAPIC]: Initialized I/OAPIC: ID: %x, Version: %x, n_redirection entries: %d\n", this->id, this->version, this->max_redirection_entries);
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

void x86_64::apic::ioapic_device::set_entry(uint8_t index, uint8_t vector, x86_64::apic::ioapic_delivery_modes delivery_mode, x86_64::apic::ioapic_destination_modes destination_mode, uint8_t pin_polarity, uint8_t trigger_mode, uint8_t destination){
    uint64_t data = 0;
    data |= vector;
    switch (delivery_mode)
    {
    case x86_64::apic::ioapic_delivery_modes::FIXED:
        data |= (0b000 << 8);
        break;
    case x86_64::apic::ioapic_delivery_modes::LOW_PRIORITY:
        data |= (0b001 << 8);
        break;
    case x86_64::apic::ioapic_delivery_modes::SMI:
        data |= (0b010 << 8);
        break;
    case x86_64::apic::ioapic_delivery_modes::NMI:
        data |= (0b100 << 8);
        break;
    case x86_64::apic::ioapic_delivery_modes::INIT:
        data |= (0b101 << 8);
        break;
    case x86_64::apic::ioapic_delivery_modes::EXTINT:
        data |= (0b111 << 8);
        break;
    }
    
    switch (destination_mode)
    {
    case x86_64::apic::ioapic_destination_modes::PHYSICAL:
        data |= (0 << 11);
        break;

    case x86_64::apic::ioapic_destination_modes::LOGICAL:
        data |= (1 << 11);
        break;
    
    }
    
    data |= ((uint64_t)pin_polarity << 13);
    data |= ((uint64_t)trigger_mode << 15);
    data |= ((uint64_t)destination << 56);
    this->set_entry(index, data);
}

auto ioapics_base = types::linked_list<types::pair<uint64_t, uint64_t>>(); // base, and gsi_base
auto interrupt_overrides = types::linked_list<x86_64::apic::interrupt_override>();

auto ioapics = types::linked_list<types::pair<x86_64::apic::ioapic_device, uint64_t>>(); // ioapic, and gsi_base

void x86_64::apic::ioapic::init(acpi::madt& madt){
    madt.get_ioapics(ioapics_base);
    madt.get_interrupt_overrides(interrupt_overrides);

    for(auto& a : ioapics_base){
        auto* ioapic = ioapics.empty_entry();
        ioapic->a.init(a.a, a.b, madt.supports_legacy_pic(), interrupt_overrides);
        ioapic->b = a.b;
    }
}

void x86_64::apic::ioapic::set_entry(uint8_t gsi, uint8_t vector, ioapic_delivery_modes delivery_mode, ioapic_destination_modes destination_mode, uint8_t pin_polarity, uint8_t trigger_mode, uint8_t destination){
    for(auto& entry : ioapics){
        if((gsi >= entry.b) && (gsi <= (entry.a.get_max_redirection_entries() + entry.b))){
            // Found correct ioapic
            entry.a.set_entry((gsi - entry.b), vector, delivery_mode, destination_mode, pin_polarity, trigger_mode, destination);
            return;
        }
    }
    debug_printf("[I/OAPIC]: Couldn't find IOAPIC with gsi: %x\n", gsi);
}

void x86_64::apic::ioapic::mask_gsi(uint8_t gsi){
    for(auto& entry : ioapics){
        if((gsi >= entry.b) && (gsi <= (entry.a.get_max_redirection_entries() + entry.b))){
            // Found correct ioapic
            entry.a.set_entry((gsi - entry.b), (entry.a.read_entry((gsi - entry.b)) | (1 << 16))); // Mask
            return;
        }
    }
}

void x86_64::apic::ioapic::unmask_gsi(uint8_t gsi){
    for(auto& entry : ioapics){
        if((gsi >= entry.b) && (gsi <= (entry.a.get_max_redirection_entries() + entry.b))){
            // Found correct ioapic
            entry.a.unmask((gsi - entry.b));
            return;
        }
    }
}