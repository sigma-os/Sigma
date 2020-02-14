#ifndef SIGMA_GENERIC_DEVICE_H
#define SIGMA_GENERIC_DEVICE_H

#include <Sigma/common.h>
#include <Sigma/types/vector.h>
#include <klibc/stdio.h>
#include <lai/core.h>
#include <Sigma/arch/x86_64/drivers/pci.h>

namespace generic::device {
    struct device {
        const char* name;
        misc::uuid id;

        struct {
            uint64_t acpi : 1;
            uint64_t pci : 1;
        } contact;

        struct {
            lai_nsnode_t* node;
            lai_api_error_t eval(lai_variable_t& var){
                LAI_CLEANUP_STATE lai_state_t state;
                lai_init_state(&state);
                return lai_eval(&var, node, &state);
            }
        } acpi_contact;

        struct {
            x86_64::pci::device* device;
        } pci_contact;

        struct resource_region { 
            enum {
                typeMmio = 0,
                typeIo,

                typeInvalid = -1
            };

            enum {
                originPciBar,
            };
            uint8_t type;
            uint64_t origin;
            uint64_t base;
            uint64_t len;
        };
        
        size_t index;
        tid_t driver;


        void add_acpi_node(lai_nsnode_t* node){
            contact.acpi = 1;
            acpi_contact.node = node;
        }

        void add_pci_device(x86_64::pci::device* device){
            contact.pci = 1;
            pci_contact.device = device;
        }
    };

    using device_descriptor = uint64_t;

    types::vector<device>& get_device_list();
    void init();
    void print_list();
    void add_pci_device(x86_64::pci::device* dev);

    constexpr uint64_t devctl_cmd_nop = 0;
    constexpr uint64_t devctl_cmd_claim = 1;
    constexpr uint64_t devctl_cmd_find_pci = 2;
    constexpr uint64_t devctl_cmd_find_pci_class = 3;
    constexpr uint64_t devctl_cmd_get_resource_region = 4;
    constexpr uint64_t devctl_cmd_enable_irq = 5;
    constexpr uint64_t devctl_cmd_wait_on_irq = 6;
    constexpr uint64_t devctl_cmd_read_pci = 7;
    constexpr uint64_t devctl_cmd_write_pci = 8;

    uint64_t devctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, x86_64::idt::idt_registers* regs);
}



#endif