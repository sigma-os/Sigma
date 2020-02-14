#pragma once

#include <libsigma/sys.h>

#include "trb.hpp"
#include "context.hpp"
#include <vector>

#include "../usb.hpp"

namespace xhci
{
    struct [[gnu::packed]] cap_regs {
        uint8_t cap_length;
        uint8_t reserved;

        union [[gnu::packed]] hci_version_t {
            struct {
                uint16_t subminor : 4;
                uint16_t minor : 4;
                uint16_t major : 8;
            };
            uint16_t raw;
        };
        static_assert(sizeof(hci_version_t) == 2);
        uint16_t hci_version;

        union [[gnu::packed]] hcs_params_1_t {
            struct {
                uint32_t max_slots : 8;
                uint32_t n_irqs : 11;
                uint32_t reserved : 5;
                uint32_t max_ports : 8;
            };
            uint32_t raw;
        };
        static_assert(sizeof(hcs_params_1_t) == 4);

        union [[gnu::packed]] hcs_params_2_t {
            struct {
                uint32_t iso_sheduling_threshold : 4;
                uint32_t erst_max : 4;
                uint32_t reserved : 13;
                uint32_t max_scratchpad_hi : 5;
                uint32_t scratchpad_restore : 1;
                uint32_t max_scratchpad_low : 5;
            };
            uint32_t raw;
        };
        static_assert(sizeof(hcs_params_2_t) == 4);

        union [[gnu::packed]] hcs_params_3_t {
            struct {
                uint32_t u1_device_exit_latency : 8;
                uint32_t reserved : 8;
                uint32_t u2_device_exit_latency : 16;
            };
            uint32_t raw;
        };
        static_assert(sizeof(hcs_params_3_t) == 4);

        uint32_t hcs_params[3];

        union [[gnu::packed]] hcc_params_1_t {
            struct {
                uint32_t addressing_64 : 1;
                uint32_t bw_negotiation : 1;
                uint32_t context_size : 1;
                uint32_t port_power_control : 1;
                uint32_t port_indicators : 1;
                uint32_t light_hc_reset : 1;
                uint32_t latency_tolerance_messaging : 1;
                uint32_t no_secondary_sid : 1;
                uint32_t parse_all_event_data : 1;
                uint32_t short_packed : 1;
                uint32_t stopped_edtla : 1;
                uint32_t contiguous_frame_id : 1;
                uint32_t max_psa_size : 4;
                uint32_t extended_cap_ptr : 16;
            };
            uint32_t raw;
        };
        static_assert(sizeof(hcc_params_1_t) == 4);
    
        uint32_t hcc_params_1;
        uint32_t doorbell_offset;
        uint32_t runtime_offset;
        uint32_t hcc_params_2;
    };

    struct [[gnu::packed]] operational_regs {
        union [[gnu::packed]] usbcmd {
            struct {
                uint32_t run : 1;
                uint32_t reset : 1;
                uint32_t irq_enable : 1;
                uint32_t host_system_error_enable : 1;
                uint32_t reserved : 3;
                uint32_t light_host_controller_reset : 1;
                uint32_t controller_save_state : 1;
                uint32_t controller_restore_state : 1;
                uint32_t enable_wrap_event : 1;
                uint32_t enable_u3_stop : 1;
                uint32_t reserved_0 : 1;
                uint32_t cem_enable : 1;
                uint32_t ext_tbc_enable : 1;
                uint32_t ext_tbc_status_enable : 1;
                uint32_t vtio_enable : 1;
                uint32_t reserved_1 : 15;
            };
            uint32_t raw;
        };
        static_assert(sizeof(usbcmd) == 4);
        uint32_t command;

        union [[gnu::packed]] usbsts {
            struct {
                uint32_t halted : 1;
                uint32_t reserved : 1;
                uint32_t host_system_error : 1;
                uint32_t event_irq : 1;
                uint32_t port_change : 1;
                uint32_t reserved_0 : 3;
                uint32_t save_state_status : 1;
                uint32_t restore_state_status : 1;
                uint32_t save_restore_error : 1;
                uint32_t controller_not_ready : 1;
                uint32_t host_controller_error : 1;
            };
            uint32_t raw;
        };
        static_assert(sizeof(usbsts) == 4);
        uint32_t status;
        uint32_t page_size;
        uint8_t reserved[8];
        uint32_t notification_control;

        union [[gnu::packed]] crcr {
            struct {
                uint64_t ring_cycle_state : 1;
                uint64_t command_stop : 1;
                uint64_t command_abort : 1;
                uint64_t command_ring_running : 1;
                uint64_t reserved : 2;
                uint64_t command_ring_ptr : 58;
            };
            uint64_t raw;
        };
        static_assert(sizeof(crcr) == 8);
        uint64_t command_ring_control;
        uint8_t reserved_0[16];
        uint64_t device_context_base;

        union [[gnu::packed]] config_reg {
            struct {
                uint32_t slot_enable : 8;
                uint32_t u3_entry_enable : 1;
                uint32_t configuration_info_enable : 1;
                uint32_t reserved : 22;
            };
            uint32_t raw;
        };
        static_assert(sizeof(config_reg) == 4);
        uint32_t config;
        uint8_t reserved_1[0x3C4];
        struct [[gnu::packed]] port_regs{
            union [[gnu::packed]] control_reg {
                struct {
                    uint32_t connect_status : 1;
                    uint32_t enabled : 1;
                    uint32_t reserved : 1;
                    uint32_t over_current_active : 1;
                    uint32_t port_reset : 1;
                    uint32_t port_link_state : 4;
                    uint32_t port_power : 1;
                    uint32_t port_speed : 4;
                    uint32_t port_indicator_control : 2;
                    uint32_t port_link_state_write_strobe : 1;
                    uint32_t connect_status_change : 1;
                    uint32_t port_enabled_change : 1;
                    uint32_t warm_port_reset_change : 1;
                    uint32_t over_current_change : 1;
                    uint32_t port_reset_change : 1;
                    uint32_t port_link_state_change : 1;
                    uint32_t port_config_error_change : 1;
                    uint32_t cold_attach_status : 1;
                    uint32_t wake_on_connect_enable : 1;
                    uint32_t wake_on_disconnect_enable : 1;
                    uint32_t wake_on_over_current_enable : 1;
                    uint32_t reserved_0 : 2;
                    uint32_t device_removable : 1;
                    uint32_t warm_port_reset : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(control_reg) == 4);
            uint32_t control;
            uint32_t power_management;
            uint32_t link_info;
            uint32_t lpm_control;
        }; 
        port_regs ports[0xFF];
    };

    struct [[gnu::packed]] runtime_regs {
        uint32_t microframe_index;
        uint8_t reserved[0x1C];
        struct [[gnu::packed]] interrupter_t {
            union [[gnu::packed]] iman_t {
                struct {
                    uint32_t irq_pending : 1;
                    uint32_t irq_enable : 1;
                    uint32_t reserved : 30;
                };
                uint32_t raw;
            };
            static_assert(sizeof(iman_t) == 4);
            uint32_t iman;

            union [[gnu::packed]] imod_t {
                struct {
                    uint32_t interval : 16;
                    uint32_t counter : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(imod_t) == 4);
            uint32_t imod;

            union [[gnu::packed]] erst_size_t {
                struct {
                    uint32_t size : 16;
                    uint32_t reserved : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(erst_size_t) == 4);
            uint32_t erst_size;
            uint32_t reserved;

            uint64_t erst_base;

            union [[gnu::packed]] erst_dequeue_t {
                struct {
                    uint64_t segment_index : 3;
                    uint64_t event_handler_busy : 1;
                    uint64_t dequeue : 60;
                };
                uint64_t raw;
            };
            static_assert(sizeof(erst_dequeue_t) == 8);
            uint64_t erst_dequeue;
        };
        interrupter_t interrupter[1024];
    };
    
    union [[gnu::packed]] doorbell_reg {
        struct {
            uint32_t target : 8;
            uint32_t reserved : 8;
            uint32_t task_id : 16;
        };
        uint32_t raw;
    };

    struct [[gnu::packed]] event_ring_segment_table_entry {
        uint64_t base;
        uint16_t size;
        uint16_t reserved;
        uint32_t reserved_0;
    };

    namespace extended_caps
    {
        struct [[gnu::packed]] bios_os_handoff {
            union [[gnu::packed]] capability_reg {
                struct {
                    uint32_t cap_id : 8;
                    uint32_t next_cap : 8;
                    uint32_t bios_owned_semaphore : 1;
                    uint32_t reserved : 7;
                    uint32_t os_owned_semaphore : 1;
                    uint32_t reserved_1 : 7;
                };
                uint32_t raw;
            };
            static_assert(sizeof(capability_reg) == 4);
            uint32_t capability;

            union [[gnu::packed]] control_reg {
                struct {
                    uint32_t smi_enable: 1;
                    uint32_t reserved_2 : 3;
                    uint32_t smi_host_system_error_enable : 1;
                    uint32_t reserved_3 : 8;
                    uint32_t smi_on_os_owner : 1;
                    uint32_t smi_on_pci_command_enable : 1;
                    uint32_t smi_on_bar_enable : 1;
                    uint32_t smi_on_event_irq : 1;
                    uint32_t reserved_4 : 3;
                    uint32_t smi_on_host_system : 1;
                    uint32_t reserved_5 : 8;
                    uint32_t smi_on_os_ownership_change : 1;
                    uint32_t smi_on_pci_command : 1;
                    uint32_t smi_on_bar : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(control_reg) == 4);
            uint32_t command;
        };

        struct [[gnu::packed]] supported_protocol_cap {
            union [[gnu::packed]] version_t {
                struct {
                    uint32_t cap_id : 8;
                    uint32_t next : 8;
                    uint32_t minor_rev : 8;
                    uint32_t major_rev : 8;
                };
                uint32_t raw;
            };
            static_assert(sizeof(version_t) == 4);
            uint32_t version;
            
            union [[gnu::packed]] name_t {
                struct {
                    uint8_t name_string[4];
                };
                uint32_t raw;
            };
            static_assert(sizeof(name_t) == 4);
            uint32_t name;

            
            union [[gnu::packed]] ports_info_t {
                struct {
                    uint32_t compatible_port_offset : 8;
                    uint32_t compatible_port_count : 8;
                    uint32_t protocol_defined : 12;
                    uint32_t protocol_speed_id_count : 4;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ports_info_t) == 4);
            uint32_t ports_info;

            union [[gnu::packed]] slot_type_t {
                struct {
                    uint32_t protocol_slot_type : 5;
                    uint32_t reserved : 27;
                };
                uint32_t raw;
            };
            static_assert(sizeof(slot_type_t) == 4);
            uint32_t slot_type;
            
            struct [[gnu::packed]] {
                uint32_t speed_id_value : 4;
                uint32_t speed_id_exponent : 2;
                uint32_t speed_id_type : 2;
                uint32_t full_duplex : 1;
                uint32_t rserved : 5;
                uint32_t link_protocol : 2;
                uint32_t speed_id_mantissa : 16;
            } speed_ids[];
            static_assert(sizeof(speed_ids[0]) == 4);
        };
    } // namespace extended_caps

    enum {
        speedFull = 1,
        speedLow = 2,
        speedHi = 3,
        speedSuper = 4,
    };
    
    

    class controller {
        public:
        controller(uint64_t device_descriptor);

        private:
        struct supported_protocol {
            uint32_t major, minor;

            uint32_t port_offset, port_count;
            uint32_t slot_type;
        };
        std::vector<supported_protocol> supported_protocols;

        struct port {
            supported_protocol* protocol;
            uint32_t offset;
            bool active;
            bool has_pair;
            uint32_t other_port_num;

            uint32_t slot_id;
            uint64_t packet_size;

            int speed;

            in_context in_ctx;
            out_context out_ctx;

            transfer_ring command;
        };

        void do_bios_os_handoff();
        void intel_enable_xhci_ports();

        void reset_controller();

        void stop_execution();
        void start_execution();

        command_completion_event_trb send_command(raw_trb* trb);

        void handle_irq();


        // Port functions
        void get_descriptor(size_t p);


        void setup_port_protocols();

        void init_ports();

        bool reset_port(size_t p);

        void send_control(size_t p, usb::packet packet, uint8_t* data, int write);

        volatile cap_regs* capabilities;
        volatile operational_regs* operation;
        volatile runtime_regs* runtime;
        volatile uint32_t* doorbells;

        volatile extended_caps::bios_os_handoff* bios_os_handoff;

        volatile uint64_t* dcbaap;
        volatile event_ring_segment_table_entry* erst;


        
        std::vector<port> ports;

        std::unordered_map<uint32_t, port*> port_by_slot_id;

        size_t page_size;
        size_t n_scratchpads;
        size_t n_ports;
        size_t dcbaap_size;
        size_t context_size;

        command_ring cmd_ring;
        event_ring evt_ring;

        uint64_t device_descriptor;
        uint64_t irq_descriptor;

        enum {
            quirkIntel = (1 << 0)
        };
        uint64_t quirks;
    };
} // namespace xhci
