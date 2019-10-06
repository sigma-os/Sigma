#include <Sigma/smp/ipi.h>

static uint64_t shootdown_addr = 0;
static uint64_t shootdown_length = 0;

auto shootdown_mutex = x86_64::spinlock::mutex();

void smp::ipi::send_shootdown(uint64_t address, uint64_t length){
    shootdown_mutex.acquire();
    shootdown_addr = address;
    shootdown_length = length;
    debug_printf("[IPI]: Requested TLB shootdown on addr: %x, length: %x\n", shootdown_addr, shootdown_length);
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | smp::ipi::shootdown_ipi_vector)); // All including self
    shootdown_mutex.release();
}

void smp::ipi::send_ping(uint8_t apic_id){
    smp::cpu::get_current_cpu()->lapic.send_ipi(apic_id, smp::ipi::ping_ipi_vector);
}

void smp::ipi::send_ping(){
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | (1 << 18) | smp::ipi::ping_ipi_vector)); // All excluding self
}

static void shootdown_ipi(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs) {
	for(uint64_t offset = 0; offset < shootdown_length; offset += mm::pmm::block_size) {
		mm::vmm::kernel_vmm::get_instance().get_paging_provider().invalidate_addr(shootdown_addr + offset);
	}
}

static void ping_ipi(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs) {
	debug_printf("[IPI]: Pong from cpu: %x\n", smp::cpu::get_current_cpu()->lapic_id);
}

void smp::ipi::init_ipi(){
    register_interrupt_handler(smp::ipi::ping_ipi_vector, ping_ipi, true);
    register_interrupt_handler(smp::ipi::shootdown_ipi_vector, shootdown_ipi, true);
}