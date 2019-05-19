#ifndef SIGMA_KERNEL_MULTIPROCESSOR
#define SIGMA_KERNEL_MULTIPROCESSOR

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/drivers/bios.h>

#include <klibc/stdio.h>
#include <klibc/string.h>

#include <Sigma/smp/smp.h>

namespace x86_64::mp
{
    constexpr const char* mp_floating_header_signature = "_MP_";

    constexpr uint8_t mp_feature_byte2_imcrp = 7;

    struct floating_pointer_table {
        const char signature[4];
        uint32_t physical_pointer_address;
        uint8_t length;
        uint8_t revision;
        uint8_t checksum;
        uint8_t mp_feature_byte1;
        uint8_t mp_feature_byte2;
        uint8_t mp_feature_byte3;
        uint8_t mp_feature_byte4;
        uint8_t mp_feature_byte5;
    } __attribute__((packed));

   constexpr const char* mp_configuration_table_header_signature = "PCMP";

    struct configuration_table_header {
        const char signature[4];
        uint16_t base_table_length;
        uint8_t specification_revision;
        uint8_t checksum;
        const char oem_id[8];
        const char product_id[12];
        uint32_t oem_table_pointer;
        uint16_t oem_table_size;
        uint16_t entry_count;
        uint32_t lapic_base;
        uint16_t extended_table_length;
        uint8_t extended_table_checksum;
        uint8_t reserved;
    } __attribute__((packed));

    struct configuration_table_entry_type {
        constexpr configuration_table_entry_type(uint8_t type, uint8_t size, const char *name):  type(type), size(size), name(name){}
        uint8_t type;
        uint8_t size;
        const char *name;
    };

    constexpr uint8_t configuration_table_entry_type_processor = 0;
    constexpr uint8_t configuration_table_entry_type_bus = 1;
    constexpr uint8_t configuration_table_entry_type_ioapic = 2;
    constexpr uint8_t configuration_table_entry_type_io_interrupt_assignment = 3;
    constexpr uint8_t configuration_table_entry_type_local_interrupt_assignment = 4;

    constexpr uint8_t configuration_table_entry_type_address_space_mapping = 128;
    constexpr uint8_t configuration_table_entry_type_bus_hierarchy_descriptor = 129;
    constexpr uint8_t configuration_table_entry_type_compatibility_bus_addressspace_modifier = 130;

    constexpr uint8_t configuration_table_entry_size_processor = 20;
    constexpr uint8_t configuration_table_entry_size_bus = 8;
    constexpr uint8_t configuration_table_entry_size_ioapic = 8;
    constexpr uint8_t configuration_table_entry_size_io_interrupt_assignment = 8;
    constexpr uint8_t configuration_table_entry_size_local_interrupt_assignment = 8;

    constexpr uint8_t configuration_table_entry_size_address_space_mapping = 20;
    constexpr uint8_t configuration_table_entry_size_bus_hierarchy_descriptor = 8;
    constexpr uint8_t configuration_table_entry_size_compatibility_bus_addressspace_modifier = 8;


    constexpr configuration_table_entry_type entry_types[] = {
        {configuration_table_entry_type_processor, configuration_table_entry_size_processor, "Processor"},
        {configuration_table_entry_type_bus, configuration_table_entry_size_bus, "Bus"},
        {configuration_table_entry_type_ioapic, configuration_table_entry_size_ioapic, "I/O APIC"},
        {configuration_table_entry_type_io_interrupt_assignment, configuration_table_entry_size_io_interrupt_assignment, "I/O Int Assignment"},
        {configuration_table_entry_type_local_interrupt_assignment, configuration_table_entry_size_local_interrupt_assignment, "Local Int Assignment"}
    };

    struct configuration_table_entry_processor {
        uint8_t entry_type; // = configuration_table_entry_type_processor
        uint8_t lapic_id;
        uint8_t lapic_version;
        uint8_t cpu_flags;
        uint8_t cpu_signature_stepping : 4;
        uint8_t cpu_signature_model : 4;
        uint8_t cpu_signature_family : 4;

        uint8_t reserved_0 : 4;

        uint16_t reserved_1;

        uint32_t feature_flags;


        uint32_t reserved_2;
        uint32_t reserved_3;
    } __attribute__((packed));

    static_assert(sizeof(configuration_table_entry_processor) == configuration_table_entry_size_processor);

    constexpr uint8_t configuration_table_entry_processor_feature_flags_bit_fpu = 0;
    constexpr uint8_t configuration_table_entry_processor_feature_flags_bit_mce = 7;
    constexpr uint8_t configuration_table_entry_processor_feature_flags_bit_cx8 = 8;
    constexpr uint8_t configuration_table_entry_processor_feature_flags_bit_apic = 9;

    struct configuration_table_entry_bus {
        uint8_t entry_type; // = configuration_table_entry_type_bus
        uint8_t bus_id;
        const char bus_type_string[6];
    } __attribute__((packed));

    static_assert(sizeof(configuration_table_entry_bus) == configuration_table_entry_size_bus);




    struct configuration_table_entry_ioapic {
        uint8_t entry_type; // = configuration_table_entry_type_ioapic
        uint8_t ioapic_id;
        uint8_t ioapic_version;
        uint8_t ioapic_flags;
        uint32_t ioapic_base_addr;
    } __attribute__((packed));
    
    static_assert(sizeof(configuration_table_entry_ioapic) == configuration_table_entry_size_ioapic);

    constexpr uint8_t configuration_table_entry_ioapic_flags_bit_en = 0;



    struct configuration_table_entry_io_interrupt_assignment {
        uint8_t entry_type; // = configuration_table_entry_type_io_interrupt_assignment
        uint8_t interrupt_type;

        uint8_t polarity : 2;
        uint8_t trigger_mode : 2;
        uint8_t reserved_0 : 4;
        uint8_t reserved_1;

        uint8_t source_bus_id;
        uint8_t source_bus_irq;
        uint8_t destination_ioapic_id;
        uint8_t destination_ioapic_intin;
    } __attribute__((packed));

    static_assert(sizeof(configuration_table_entry_io_interrupt_assignment) == configuration_table_entry_size_io_interrupt_assignment);

    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_interrupt_type_int = 0;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_interrupt_type_nmi = 1;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_interrupt_type_smi = 2;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_interrupt_type_extint = 3;

    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_polarity_conforming = 0;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_polarity_active_high = 1;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_polarity_reserved = 2;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_polarity_active_low = 3;

    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_trigger_mode_conforming = 0;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_trigger_mode_edge_triggered = 1;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_trigger_mode_reserved = 2;
    constexpr uint8_t configuration_table_entry_io_interrupt_assignment_trigger_mode_level_triggered = 3;


    struct configuration_table_entry_local_interrupt_assignment {
        uint8_t entry_type; // = configuration_table_entry_type_local_interrupt_assignment
        uint8_t interrupt_type;

        uint8_t polarity : 2;
        uint8_t trigger_mode : 2;
        uint8_t reserved_0 : 4;
        uint8_t reserved_1;

        uint8_t source_bus_id;
        uint8_t source_bus_irq;
        uint8_t destination_lapic_id;
        uint8_t destination_lapic_lintin;
    } __attribute__((packed));

    static_assert(sizeof(configuration_table_entry_local_interrupt_assignment) == configuration_table_entry_size_local_interrupt_assignment);

    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_interrupt_type_int = 0;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_interrupt_type_nmi = 1;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_interrupt_type_smi = 2;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_interrupt_type_extint = 3;

    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_polarity_conforming = 0;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_polarity_active_high = 1;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_polarity_reserved = 2;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_polarity_active_low = 3;

    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_trigger_mode_conforming = 0;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_trigger_mode_edge_triggered = 1;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_trigger_mode_reserved = 2;
    constexpr uint8_t configuration_table_entry_local_interrupt_assignment_trigger_mode_level_triggered = 3;

    struct configuration_table_entry_address_space_mapping {
        uint8_t entry_type; // = configuration_table_entry_type_local_interrupt_assignment
        uint8_t entry_length;

        uint8_t bus_id;
        uint8_t address_type;
        uint64_t base;
        uint64_t length;
    } __attribute__((packed));

    constexpr uint8_t configuration_table_entry_address_space_mapping_address_type_io = 0;
    constexpr uint8_t configuration_table_entry_address_space_mapping_address_type_memory = 1;
    constexpr uint8_t configuration_table_entry_address_space_mapping_address_type_prefetch = 2;

    struct configuration_table_entry_bus_hierarchy_descriptor {
        uint8_t entry_type; // = configuration_table_entry_type_local_interrupt_assignment
        uint8_t entry_length;

        uint8_t bus_id;
        uint8_t bus_information;
        uint8_t parent_bus;
    } __attribute__((packed));

    constexpr uint8_t configuration_table_entry_bus_hierarchy_descriptor_bus_information_subtractive_decode_bit = 0;

    struct configuration_table_entry_compatibility_bus_addressspace_modifier {
        uint8_t entry_type; // = configuration_table_entry_type_local_interrupt_assignment
        uint8_t entry_length;

        uint8_t bus_id;
        uint8_t address_modifier;
        uint32_t predefined_range_list;
    } __attribute__((packed));

    constexpr uint8_t configuration_table_entry_compatibility_bus_addressspace_modifier_address_modifier_subtract_bit = 0;

    constexpr uint32_t configuration_table_entry_compatibility_bus_addressspace_modifier_predefined_range_list_isa_io = 0;
    constexpr uint32_t configuration_table_entry_compatibility_bus_addressspace_modifier_predefined_range_list_vga_io = 1;

    class mp {
        public:
        explicit mp(types::linked_list<smp::cpu_entry>& cpus);

        private:
        void parse(types::linked_list<smp::cpu_entry>& cpus);

        void parse_bus(uint64_t pointer);
        void parse_cpu(uint64_t pointer, types::linked_list<smp::cpu_entry>& cpus);
        void parse_ioapic(uint64_t pointer);
        void parse_io_interrupt_entry(uint64_t pointer);
        void parse_local_interrupt_entry(uint64_t pointer);

        void parse_address_space_mapping(uint64_t pointer);
        void parse_bus_hierarchy_descriptor(uint64_t pointer);
        void parse_compatibility_bus_addressspace_modifier(uint64_t pointer);

        bool check_floating_pointer();
        bool check_table_header();
        bool check_extended_table();



        floating_pointer_table* find_pointer();


        uint64_t floating_pointer;
        uint64_t table;
    };

} // x86_64::mp




#endif  