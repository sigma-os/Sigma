#include <Sigma/smp/ipi.h>

static uint64_t shootdown_addr;
static uint64_t shootdown_length;

void smp::ipi::send_shootdown(uint64_t address, uint64_t length){
    shootdown_addr = address;
    shootdown_length = length;
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | (1 << 18) | smp::ipi::ping_ipi_vector)); // All excluding self

    // TODO: Shoot ourselves down
}

void smp::ipi::send_ping(uint8_t apic_id){
    smp::cpu::get_current_cpu()->lapic.send_ipi(apic_id, smp::ipi::ping_ipi_vector);
}

void smp::ipi::send_ping(){
    smp::cpu::get_current_cpu()->lapic.send_ipi_raw(0, ((1 << 19) | (1 << 18) | smp::ipi::ping_ipi_vector)); // All excluding self
}


static void shootdown_ipi(x86_64::idt::idt_registers* regs){
    UNUSED(regs);
    debug_printf("[IPI]: Requested TLB shootdown on addr: %x, length: %x\n TODO: Implement\n", shootdown_addr, shootdown_length);
}

static void ping_ipi(x86_64::idt::idt_registers* regs){
    UNUSED(regs);
    debug_printf("[IPI]: Pong from cpu: %x\n", smp::cpu::get_current_cpu()->lapic_id);
}

void smp::ipi::init_ipi(){
    register_interrupt_handler(smp::ipi::ping_ipi_vector, ping_ipi, true);
    register_interrupt_handler(smp::ipi::shootdown_ipi_vector, shootdown_ipi, true);
}