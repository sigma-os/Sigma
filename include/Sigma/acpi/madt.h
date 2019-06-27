#ifndef SIGMA_KERNEL_ACPI_MADT
#define SIGMA_KERNEL_ACPI_MADT

#include <Sigma/common.h>
#include <Sigma/acpi/acpi.h>
#include <Sigma/types/linked_list.h>
#include <Sigma/types/pair.h>
#include <Sigma/smp/smp.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/arch/x86_64/drivers/apic.h>

namespace acpi
{
    constexpr const char* madt_signature = "APIC";

    struct madt_header
    {
        acpi::sdt_header header;
        uint32_t lapic_addr;
        uint32_t flags;
    } __attribute__((packed)); // Table of entries next

    constexpr uint8_t flags_pc_at_compatibility = 0; // Has old PIC
    
    constexpr uint8_t type_lapic = 0;
    constexpr uint8_t type_ioapic = 1;
    constexpr uint8_t type_interrupt_source_override = 2;
    constexpr uint8_t type_nmi_source = 3;

    struct madt_lapic
    {
        uint8_t type; // 0
        uint8_t length; // 8
        uint8_t acpi_uid;
        uint8_t apic_id;
        uint32_t flags;
    } __attribute__((packed));
    static_assert(sizeof(madt_lapic) == 8);
    
    constexpr uint32_t madt_lapic_flags_enabled = 0;
    constexpr uint32_t madt_lapic_flags_online_capable = 1;

    struct madt_ioapic
    {
        uint8_t type; // 1
        uint8_t length; // 12
        uint8_t apic_id;
        uint8_t reserved;
        uint32_t ioapic_addr;
        uint32_t gsi_base;
    } __attribute__((packed));
    static_assert(sizeof(madt_ioapic) == 12);

    struct madt_gsi_override {
        uint8_t type; // 2
        uint8_t length; // 10
        uint8_t bus; // 0, ISA
        uint8_t source;
        uint32_t gsi;
        uint16_t flags;

        uint8_t get_polatity(){
            return (this->flags & 0b11);
        }

        uint8_t get_trigger_mode(){
            return ((this->flags >> 2) & 0b11);
        }
    } __attribute__((packed));
    static_assert(sizeof(madt_gsi_override) == 10);

    struct madt_nmi_souce{
        uint8_t type; // 8
        uint8_t length; // 8;
        uint16_t flags;
        uint32_t gsi;
    } __attribute__((packed));
    static_assert(sizeof(madt_nmi_souce) == 8);
    




    class madt {
        public:
        madt();

        void parse();

        void get_cpus(types::linked_list<smp::cpu_entry>& cpus);
        void get_ioapics(types::linked_list<types::pair<uint64_t, uint64_t>>& ioapics);
        void get_interrupt_overrides(types::linked_list<x86_64::apic::interrupt_override>& ioapics);
        uint64_t get_lapic_address();
        bool supports_legacy_pic(){
            return this->legacy_pic;
        }

        bool found_table(){
            return (this->table == nullptr) ? (false) : (true);
        }

        private:

        void parse_lapic(uint8_t* item);
        void parse_ioapic(uint8_t* item);
        void parse_iso(uint8_t* item);

        bool legacy_pic;
        types::linked_list<smp::cpu_entry> cpus;
        types::linked_list<types::pair<uint64_t, uint64_t>> ioapics;
        types::linked_list<x86_64::apic::interrupt_override> isos;
        madt_header* table;
    };


} // namespace acpi


#endif