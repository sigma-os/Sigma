#ifndef SIGMA_KERNEL_SMP_IPI
#define SIGMA_KERNEL_SMP_IPI

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/idt.h>

namespace smp
{
    namespace ipi
    {
        constexpr uint8_t ping_ipi_vector = 250;
        constexpr uint8_t shootdown_ipi_vector = 251;

        void send_shootdown(uint64_t address, uint64_t length);
        void send_ping(uint8_t apic_id);
        void send_ping();

        void init_ipi();
    } // namespace ipi
} // namespace spm

#endif