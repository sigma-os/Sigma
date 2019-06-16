#include <Sigma/arch/x86_64/drivers/apic.h>

uint32_t x86_64::apic::lapic::read(uint32_t reg){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_VBASE + reg);
    return *val;
}

void x86_64::apic::lapic::write(uint32_t reg, uint32_t data){
    volatile uint32_t* val = reinterpret_cast<uint32_t*>(this->base + KERNEL_VBASE + reg);
    *val = data;
}

void x86_64::apic::lapic::init(){
    uint64_t apic_base_msr = msr::read(msr::apic_base);

    base = (apic_base_msr & 0xFFFFFFFFFFFFF000);

    bitops<uint64_t>::bit_set(apic_base_msr, 11); // Set Enable bit
    msr::write(msr::apic_base, apic_base_msr);

    mm::vmm::kernel_vmm::get_instance().map_page(base, (base + KERNEL_VBASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);

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

auto timer_mutex = x86_64::spinlock::mutex();

void x86_64::apic::lapic::enable_timer(uint8_t vector, uint64_t ms, x86_64::apic::lapic_timer_modes mode){  
    x86_64::spinlock::acquire(&timer_mutex);
    uint32_t ticks_per_second;

    if(this->timer_ticks_per_ms != 0) goto finalize;

    this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16

    x86_64::cmos::sleep_second(); // Align sleep time

    this->write(x86_64::apic::lapic_timer_initial_count, 0xFFFFFFFF);
    this->set_timer_mask(false);

    x86_64::cmos::sleep_second(); // Time 1 second

    this->set_timer_mask(true);

    ticks_per_second = 0xFFFFFFFF - this->read(x86_64::apic::lapic_timer_current_count);
    this->timer_ticks_per_ms = ticks_per_second / 1000; // Seconds to milliseconds

finalize:
    this->write(x86_64::apic::lapic_timer_divide_configuration, 0x3); // Use divider 16
    this->set_timer_mode(mode);
    this->set_timer_vector(vector);
    this->write(x86_64::apic::lapic_timer_initial_count, this->timer_ticks_per_ms * ms);

    this->set_timer_mask(false);

    x86_64::spinlock::release(&timer_mutex);
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