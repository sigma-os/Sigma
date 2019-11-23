#ifndef SIGMA_PROC_DEVICE_H
#define SIGMA_PROC_DEVICE_H

#include <Sigma/common.h>
#include <Sigma/types/vector.h>
#include <klibc/stdio.h>
#include <lai/core.h>
#include <Sigma/arch/x86_64/drivers/pci.h>

namespace proc::device {
    struct uuid {
        std::byte data[16];
    };

    struct device {
        const char* name;
        uuid id;

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
            static constexpr uint8_t type_mmio = 0;
            static constexpr uint8_t type_io = 1;
            static constexpr uint8_t type_invalid = 0xFF;       
            uint8_t type;
            uintptr_t base;
            size_t len;
        };
        
        types::vector<resource_region> resources;
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
    device_descriptor find_acpi_node(lai_nsnode_t* node);
    device_descriptor find_pci_node(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function);
    bool get_resource_region(device_descriptor dev, uint8_t index, device::resource_region* data);

    constexpr uint64_t devctl_cmd_nop = 0;
    constexpr uint64_t devctl_cmd_claim = 1;
    constexpr uint64_t devctl_cmd_find_pci = 2;
    constexpr uint64_t devctl_cmd_get_resource_region = 3;

    uint64_t devctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
}



#endif