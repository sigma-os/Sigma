#ifndef SIGMA_KERNEL_X86_64_HPET
#define SIGMA_KERNEL_X86_64_HPET

#include <Sigma/common.h>
#include <Sigma/mm/vmm.h>
#include <Sigma/acpi/acpi.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/idt.h>

namespace x86_64::hpet
{
    struct PACKED_ATTRIBUTE table {
        acpi::sdt_header header;
        uint32_t event_timer_block_id;
        acpi::generic_address_structure base_addr_low;
        uint8_t hpet_number;
        uint16_t main_counter_minimum_periodic_clock_tick;
        uint8_t page_protection;
    };

    constexpr const char* hpet_pnp_id = "PNP0103";

    constexpr uint64_t general_capabilities_and_id_reg = 0x0;
    constexpr uint64_t general_configuration_reg = 0x10;
    constexpr uint64_t general_interrupt_status_reg = 0x20;
    constexpr uint64_t main_counter_reg = 0xF0;

    #define hpet_timer_configuration_and_capabilities_reg(n) (0x100 + (0x20 * (n)))
    #define hpet_timer_comparator_value_reg(n) (0x108 + (0x20 * (n)))
    #define hpet_timer_fsb_interrupt_routing_reg(n) (0x110 + (0x20 * (n)))

    constexpr uint64_t femto_per_nano = 1000000;
    constexpr uint64_t femto_per_micro = femto_per_nano * 1000;
    constexpr uint64_t femto_per_milli = femto_per_micro * 1000;

    void init_hpet();

    void poll_sleep(uint64_t ms);

    enum class hpet_timer_types {ONE_SHOT, PERIODIC};

    bool create_timer(uint8_t comparator, x86_64::hpet::hpet_timer_types type, uint8_t vector, uint64_t ms);
} // namespace x86_64::hpet



#endif