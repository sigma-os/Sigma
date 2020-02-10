#ifndef SIGMA_ARCH_X86_64_INTEL_VT_D_H
#define SIGMA_ARCH_X86_64_INTEL_VT_D_H

#include <Sigma/common.h>
#include <Sigma/acpi/acpi.h>
#include <Sigma/types/vector.h>

#include <Sigma/arch/x86_64/intel/sl_paging.hpp>
#include <Sigma/types/hash_map.hpp>
#include <Sigma/types/bitmap.hpp>

namespace x86_64::vt_d
{
    namespace acpi_table {
        struct PACKED_ATTRIBUTE dmar {
            acpi::sdt_header header;
            uint8_t host_dma_address_width;
            struct {
                uint8_t irq_remap : 1;
                uint8_t x2apic_opt_out : 1;
                uint8_t dma_control_opt_in : 1;
                uint8_t reserved : 5;
            } flags;
            static_assert(sizeof(flags) == 1);
            uint8_t reserved[10];
        };

        constexpr uint16_t dma_remapping_def_id = 0;
        constexpr uint16_t reserved_mem_region_id = 1;
        constexpr uint16_t root_port_ats_cap_id = 2;
        constexpr uint16_t remapping_hw_static_affinity = 3;
        constexpr uint16_t acpi_ns_device_decl_id = 4;

        struct PACKED_ATTRIBUTE device_scope {
            uint8_t type;
            uint8_t length;
            uint16_t reserved;
            uint8_t enum_id;
            uint8_t start_bus_num;
        };

        struct PACKED_ATTRIBUTE dma_remapping_def {
            uint16_t type;
            uint16_t length;
            struct {
                uint8_t include_pci_all : 1;
                uint8_t reserved : 7;
            } flags;
            static_assert(sizeof(flags) == 1);

            uint8_t reserved;
            uint16_t segment_number;
            uint64_t register_base;
            device_scope devices[];
        };

        struct PACKED_ATTRIBUTE reserved_mem_region {
            uint16_t type;
            uint16_t length;
            uint16_t reserved;
            uint16_t segment_number;
            uint64_t region_base;
            uint64_t region_limit;
            device_scope devices[];
        };

        struct PACKED_ATTRIBUTE root_port_ats_cap {
            uint16_t type;
            uint16_t length;
            struct {
                uint8_t all_ports : 1;
                uint8_t reserved : 7;
            } flags;
            static_assert(sizeof(flags) == 1);
            uint8_t reserved;
            uint16_t segment_number;
            device_scope devices[];
        };

        struct PACKED_ATTRIBUTE remapping_hw_static_affinity {
            uint16_t type;
            uint16_t length;
            uint32_t reserved;
            uint64_t register_base;
            uint32_t proximity_domain;
        };

        struct PACKED_ATTRIBUTE acpi_ns_device_decl {
            uint16_t type;
            uint16_t length;
            uint8_t reserved[3];
            uint8_t acpi_device_number;
            char path[];
        };
    }

    struct PACKED_ATTRIBUTE root_table {
        struct PACKED_ATTRIBUTE {
            uint64_t present : 1;
            uint64_t reserved : 11;
            uint64_t context_table_ptr : 52;
            uint64_t reserved_0;
        } entries[0xFF];

        static_assert(sizeof(decltype(entries[0])) == 16);
    };

    struct PACKED_ATTRIBUTE context_table {
        struct PACKED_ATTRIBUTE {
            uint64_t present : 1;
            uint64_t fault_processing_disable : 1;
            uint64_t translation_type : 2;
            uint64_t reserved : 8;
            uint64_t second_level_page_translation_ptr : 52;
            uint64_t address_width : 3;
            uint64_t ignored : 4;
            uint64_t reserved_0 : 1;
            uint64_t domain_id : 16;
            uint64_t reserved_1 : 40;
        } entries[0xFF];

        static_assert(sizeof(decltype(entries[0])) == 16);
    };

    struct PACKED_ATTRIBUTE fault_recording_reg {
        uint64_t reserved : 12;
        uint64_t fault_info : 52;
        uint64_t sid : 16;
        uint64_t reserved_0 : 12;
        uint64_t type_bit_2 : 1;
        uint64_t supervisor : 1;
        uint64_t execute : 1;
        uint64_t pasid_present : 1;
        uint64_t reason : 8;
        uint64_t pasid : 20;
        uint64_t address_type : 2;
        uint64_t type_bit_1 : 1;
        uint64_t fault : 1;
    };
    static_assert(sizeof(fault_recording_reg) == 16);

    struct PACKED_ATTRIBUTE dma_remapping_engine_regs {
        uint32_t version;
        uint32_t reserved;
        uint64_t capabilites;
        uint64_t extended_capabilities;
        uint32_t global_command;
        uint32_t global_status;

        union PACKED_ATTRIBUTE root_table_address_t {
            struct PACKED_ATTRIBUTE {
                uint64_t reserved : 10;
                uint64_t translation_type : 2;
                uint64_t address : 52;
            };
            uint64_t raw;
        };
        static_assert(sizeof(root_table_address_t) == 8);
        uint64_t root_table_address;
        uint64_t context_command;
        uint32_t reserved_0;
        uint32_t fault_status;
        uint32_t fault_event_control;
        uint32_t fault_event_data;
        uint32_t fault_event_address;

        union PACKED_ATTRIBUTE fault_event_upper_address_t {
            struct PACKED_ATTRIBUTE {
                uint32_t reserved : 8;
                uint32_t upper_destination_id : 24;
            };
            uint32_t raw;
        };
        static_assert(sizeof(fault_event_upper_address_t) == 4);
        uint32_t fault_event_upper_address;
        uint64_t reserved_1;
        uint64_t reserved_2;
        uint64_t advanced_fault_log;
        uint32_t reserved_3;
        uint32_t protected_mem_enable;
        uint32_t protected_low_mem_base;
        uint32_t protected_low_mem_limit;
        uint64_t protected_high_mem_base;
        uint64_t protected_high_mem_limit;
        uint64_t invalidation_queue_head;
        uint64_t invalidation_queue_tail;
        uint64_t invalidation_queue_address;
        uint32_t reserved_4;
        uint32_t invalidation_completion_status;
        uint32_t invalidation_completion_event_control;
        uint32_t invalidation_completion_event_data;
        uint32_t invalidation_completion_event_address;
        uint32_t invalidation_completion_event_upper_address;
        uint64_t invalidation_queue_event_record;
        uint64_t irq_remapping_table_base;
        uint64_t page_request_queue_head;
        uint64_t page_request_queue_tail;
        uint64_t page_request_queue_address;
        uint32_t reserved_5;
        uint32_t page_request_status;
        uint32_t page_request_event_control;
        uint32_t page_request_event_data;
        uint32_t page_request_event_address;
        uint32_t page_request_event_upper_address;

        uint64_t mtrr_cap;
        uint64_t default_mtrr_type;
        uint64_t mttrs[11];
        struct {
            uint64_t base;
            uint64_t mask;
        } variable_mttrs[10];

        uint64_t virtual_command_capability;
        uint64_t reserved_6;
        uint64_t virtual_command;
        uint64_t reserved_7;
        uint64_t virtual_command_response;
        uint64_t reserved_8;
    };

    class device_context_table {
        public:
        device_context_table() = default;
        device_context_table(types::bitmap* domain_id_map);
        ~device_context_table();

        uint64_t get_phys();

        sl_paging::context& get_translation(uint8_t bus, uint8_t dev, uint8_t func);

        private:
        volatile root_table* root;
        uint64_t root_phys;

        struct hasher {
            using hash_result = uint16_t;
            hash_result operator()(uint16_t sid){
                return sid;
            }
        };

        types::bitmap* domain_id_map;
        types::hash_map<uint16_t, sl_paging::context*, hasher> sl_map;
    };

    class dma_remapping_engine {
        public:
        dma_remapping_engine() = default;
        dma_remapping_engine(acpi_table::dma_remapping_def* def);

        volatile dma_remapping_engine_regs* regs;
        acpi_table::dma_remapping_def* def;

        device_context_table root_table;

        volatile fault_recording_reg* fault_recording_regs;

        size_t n_fault_recording_regs;
        size_t n_domain_ids;

        types::bitmap domain_ids;
    };

    class iommu {
        public:
        iommu();

        private:
        acpi_table::dmar* table;

        types::linked_list<dma_remapping_engine> engines;
    };
} // namespace x86_64::vt_d


#endif