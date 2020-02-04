#include <Sigma/smp/ipi.h>
#include <atomic>


#pragma region tlb_shootdown

static uint64_t shootdown_addr = 0;
static uint64_t shootdown_length = 0;

auto shootdown_mutex = x86_64::spinlock::mutex();

void smp::ipi::send_shootdown(uint64_t address, uint64_t length){
    std::lock_guard guard{shootdown_mutex};
    shootdown_addr = address;
    shootdown_length = length;
    debug_printf("[IPI]: Requested TLB shootdown on addr: %x, length: %x\n", shootdown_addr, shootdown_length);
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | smp::ipi::shootdown_ipi_vector)); // All including self
}



static void shootdown_ipi(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs, MAYBE_UNUSED_ATTRIBUTE void* userptr) {
	for(uint64_t offset = 0; offset < shootdown_length; offset += mm::pmm::block_size) {
		x86_64::paging::invalidate_addr(shootdown_addr + offset);
	}
}

#pragma endregion

#pragma region ping

void smp::ipi::send_ping(uint32_t apic_id){
    smp::cpu::get_current_cpu()->lapic.send_ipi(apic_id, smp::ipi::ping_ipi_vector);
}

void smp::ipi::send_ping(){
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | (1 << 18) | smp::ipi::ping_ipi_vector)); // All excluding self
}

static void ping_ipi(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs, MAYBE_UNUSED_ATTRIBUTE void* userptr) {
	debug_printf("[IPI]: Pong from cpu: %x\n", smp::cpu::get_current_cpu()->lapic_id);
}

#pragma endregion

void smp::ipi::init_ipi(){
    x86_64::idt::register_interrupt_handler({.vector = smp::ipi::ping_ipi_vector, .callback = ping_ipi, .is_irq = true});
    x86_64::idt::register_interrupt_handler({.vector = smp::ipi::shootdown_ipi_vector, .callback = shootdown_ipi, .is_irq = true});
}