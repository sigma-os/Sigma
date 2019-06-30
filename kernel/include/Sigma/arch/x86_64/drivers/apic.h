#ifndef SIGMA_KERNEL_X86_64_APIC
#define SIGMA_KERNEL_X86_64_APIC

#include <Sigma/common.h>
#include <Sigma/mm/vmm.h>

#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/arch/x86_64/misc/spinlock.h>
#include <Sigma/arch/x86_64/drivers/cmos.h>
#include <Sigma/types/linked_list.h>

namespace acpi
{   
    class madt;
} // namespace acpi


namespace x86_64::apic
{
    constexpr uint64_t lapic_id = 0x20;
    constexpr uint64_t lapic_version = 0x30;

    constexpr uint64_t lapic_tpr = 0x80;
    constexpr uint64_t lapic_apr = 0x90;
    constexpr uint64_t lapic_ppr = 0xA0;
    constexpr uint64_t lapic_eoi = 0xB0;
    constexpr uint64_t lapic_rrd= 0xC0;
    constexpr uint64_t lapic_ldr = 0xD0;
    constexpr uint64_t lapic_dfr = 0xE0;
    constexpr uint64_t lapic_spurious = 0xF0;

    constexpr uint64_t lapic_isr0 = 0x100;
    constexpr uint64_t lapic_isr1 = 0x110;
    constexpr uint64_t lapic_isr2 = 0x120;
    constexpr uint64_t lapic_isr3 = 0x130;
    constexpr uint64_t lapic_isr4 = 0x140;
    constexpr uint64_t lapic_isr5 = 0x150;
    constexpr uint64_t lapic_isr6 = 0x160;
    constexpr uint64_t lapic_isr7 = 0x170;

    constexpr uint64_t lapic_tmr0 = 0x180;
    constexpr uint64_t lapic_tmr1 = 0x190;
    constexpr uint64_t lapic_tmr2 = 0x1A0;
    constexpr uint64_t lapic_tmr3 = 0x1B0;
    constexpr uint64_t lapic_tmr4 = 0x1C0;
    constexpr uint64_t lapic_tmr5 = 0x1D0;
    constexpr uint64_t lapic_tmr6 = 0x1E0;
    constexpr uint64_t lapic_tmr7 = 0x1F0;

    constexpr uint64_t lapic_irr0 = 0x200;
    constexpr uint64_t lapic_irr1 = 0x210;
    constexpr uint64_t lapic_irr2 = 0x220;
    constexpr uint64_t lapic_irr3 = 0x230;
    constexpr uint64_t lapic_irr4 = 0x240;
    constexpr uint64_t lapic_irr5 = 0x250;
    constexpr uint64_t lapic_irr6 = 0x260;
    constexpr uint64_t lapic_irr7 = 0x270;

    constexpr uint64_t lapic_error = 0x280;
    constexpr uint64_t lapic_lvt_cmci = 0x2F0;

    constexpr uint64_t lapic_icr_low = 0x300;
    constexpr uint64_t lapic_icr_high = 0x310;

    constexpr uint64_t lapic_lvt_timer = 0x320;
    constexpr uint64_t lapic_lvt_thermal_sensor = 0x330;
    constexpr uint64_t lapic_lvt_performance_monitoring = 0x340;
    constexpr uint64_t lapic_lvt_lint0 = 0x350;
    constexpr uint64_t lapic_lvt_lint1 = 0x360;
    constexpr uint64_t lapic_lvt_error = 0x370;

    constexpr uint64_t lapic_timer_initial_count = 0x380;
    constexpr uint64_t lapic_timer_current_count = 0x390;
    constexpr uint64_t lapic_timer_divide_configuration = 0x3E0;


    constexpr uint64_t lapic_spurious_software_enable = 8;
    constexpr uint64_t lapic_spurious_focus_processor_checking = 9;
    constexpr uint64_t lapic_spurious_eoi_broadcast_suppression = 12;


    constexpr uint64_t lapic_icr_ds_self = 0x40000;
    constexpr uint64_t lapic_icr_ds_allinc = 0x80000;
    constexpr uint64_t lapic_icr_ds_allex = 0xC0000;
    constexpr uint64_t lapic_icr_tm_level = 0x8000;
    constexpr uint64_t lapic_icr_levelassert = 0x4000;
    constexpr uint64_t lapic_icr_status_pending = 0x1000;
    constexpr uint64_t lapic_icr_dm_logical = 0x800;
    constexpr uint64_t lapic_icr_dm_lowpri = 0x100;
    constexpr uint64_t lapic_icr_dm_smi = 0x200;
    constexpr uint64_t lapic_icr_dm_nmi = 0x400;
    constexpr uint64_t lapic_icr_dm_init = 0x500;
    constexpr uint64_t lapic_icr_dm_sipi = 0x600;

    constexpr uint64_t lapic_lvt_mask = 16;

    enum class lapic_timer_modes { 
        ONE_SHOT = 0,
        PERIODIC = 1,
        TSC_DEADLINE = 2
    };

    //TODO: x2APIC support
    class lapic {
        public:
        uint8_t get_id(){
            return this->id;
        }

        uint8_t get_version(){
            return this->version;
        }

        void send_ipi_raw(uint8_t target_lapic_id, uint32_t flags);
        void send_ipi(uint8_t target_lapic_id, uint8_t vector);
        void send_eoi();

        void enable_timer(uint8_t vector, uint64_t ms, x86_64::apic::lapic_timer_modes mode);

        void init();

        lapic(): base(0), id(0), version(0), max_lvt_entries(0), timer_ticks_per_ms(0) {};

        private:
        uint64_t base;
        uint8_t id;
        uint8_t version;
        uint8_t max_lvt_entries;
        uint64_t timer_ticks_per_ms;

        uint32_t read(uint32_t reg);
        void write(uint32_t reg, uint32_t val);

        void set_timer_mode(x86_64::apic::lapic_timer_modes mode);
        void set_timer_vector(uint8_t vector);
        void set_timer_mask(bool state);
    };

    constexpr uint8_t ioapic_register_select = 0x0;
    constexpr uint8_t ioapic_register_data = 0x10;

    constexpr uint32_t ioapic_id = 0;
    constexpr uint32_t ioapic_ver = 1;
    constexpr uint32_t ioapic_arb = 2;

    constexpr uint32_t ioapic_redirection_table = 0x10;

    enum class ioapic_delivery_modes { 
        FIXED = 0b000,
        LOW_PRIORITY = 0b001,
        SMI = 0b010,
        NMI = 0b100,
        INIT = 0b101,
        EXTINT = 0b111
    };

    enum class ioapic_destination_modes { 
        PHYSICAL = 0,
        LOGICAL = 1
    };

    struct interrupt_override {
        uint8_t source;
        uint32_t gsi;
        uint8_t polarity;
        uint8_t trigger_mode;
    };

    class ioapic_device {
        public:
        uint8_t get_id(){
            return this->id;
        }
        uint8_t get_version(){
            return this->version;
        }
        uint8_t get_max_redirection_entries(){
            return this->max_redirection_entries;
        }
        ioapic_device() = default;
        void init(uint64_t base, uint32_t gsi_base, bool pic, types::linked_list<x86_64::apic::interrupt_override>& isos);
        void set_entry(uint8_t index, uint8_t vector, ioapic_delivery_modes delivery_mode, ioapic_destination_modes destination_mode, uint8_t pin_polarity, uint8_t trigger_mode, uint8_t destination);
        uint64_t read_entry(uint8_t index);
        void set_entry(uint8_t index, uint64_t data);
        void unmask(uint8_t index);
        private:
        uint8_t id;
        uint8_t max_redirection_entries;
        uint8_t version;
        uint64_t base;
        uint32_t read(uint32_t reg);
        void write(uint32_t reg, uint32_t value);        
    };

    namespace ioapic {
        void init(acpi::madt& madt);
        void set_entry(uint8_t gsi, uint8_t vector, ioapic_delivery_modes delivery_mode, ioapic_destination_modes destination_mode, uint8_t pin_polarity, uint8_t trigger_mode, uint8_t destination);
        void mask_gsi(uint8_t gsi);
        void unmask_gsi(uint8_t gsi);
    };
} // x86_64::apic

#endif