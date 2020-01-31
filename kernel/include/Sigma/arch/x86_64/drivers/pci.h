#ifndef SIGMA_KERNEL_X86_64_PCI
#define SIGMA_KERNEL_X86_64_PCI

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/io.h>
#include <Sigma/acpi/acpi.h>
#include <lai/helpers/pci.h>
#include <lai/core.h>

namespace acpi
{
    constexpr const char* mcfg_table_signature = "MCFG";

    struct PACKED_ATTRIBUTE mcfg_table_entry {
        uint64_t base;
        uint16_t seg;
        uint8_t start_bus_number;
        uint8_t end_bus_number;
        uint32_t reserved;
    };

    struct PACKED_ATTRIBUTE mcfg_table {
        acpi::sdt_header header;
        uint64_t reserved;
        mcfg_table_entry entries[1];
    };
} // namespace acpi


namespace x86_64::pci
{
    namespace msi
    {
        enum {
            msi_control_reg = 0x2,
            msi_addr_reg_low = 0x4,
            msi_data_32 = 0x8,
            msi_data_64 = 0xC,

            msi_64bit_supported = (1 << 7),
            msi_enable = (1 << 1)
        };

        union PACKED_ATTRIBUTE address {
            struct {
                uint32_t reserved : 2;
                uint32_t destination_mode : 1;
                uint32_t redirection_hint : 1;
                uint32_t _reserved_0 : 8;
                uint32_t destination_id : 8;
                // must be 0xFEE
                uint32_t base_address : 12;
            };
            uint32_t raw;
        };

        union PACKED_ATTRIBUTE data {
            struct {
                uint32_t vector : 8;
                uint32_t delivery_mode : 3;
                uint32_t reserved : 3;
                uint32_t level : 1;
                uint32_t trigger_mode : 1;
                uint32_t reserved_0 : 16;
            };
            uint32_t raw;
        };
    } // namespace msi
    

    constexpr uint16_t config_addr = 0xCF8;
    constexpr uint16_t config_data = 0xCFC;



    uint32_t read(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size);
    uint32_t read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size);
    void write(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size);
    void write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size);

    
    struct bar {
        uint8_t type;
        uint8_t number;
        uint8_t flags;
        uint64_t base;
        uint64_t len;
    };

    constexpr uint8_t bar_type_mem = 0;
    constexpr uint8_t bar_type_io = 1;
    constexpr uint8_t bar_type_invalid = 0xFF;

    constexpr uint8_t bar_flags_prefetchable = 0;

    x86_64::pci::bar read_bar(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint8_t number);
    x86_64::pci::bar read_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t number);

    struct device {
        bool exists;
        bool is_bridge;
        uint16_t seg;
        uint8_t bus, device, function;
        uint16_t vendor_id;
        uint8_t header_type, class_code, subclass_code, prog_if;

        x86_64::pci::device* parent;
        lai_nsnode_t* node;
        lai_variable_t prt;

        struct {
            bool supported;
            uint8_t space_offset;
        } msi;

        struct {
            bool supported;
            uint8_t space_offset;
        } msix;

        struct {
            bool supported;
            uint8_t minor_ver, major_ver;
            uint8_t speed_multiplier;
        } agp;


        bool has_irq;
        uint32_t gsi;
        x86_64::pci::bar bars[6];

        void install_msi(uint32_t dest_id, uint8_t vector);
    };

    using pci_iterator = uint64_t;

    x86_64::pci::device iterate(pci_iterator& iterator);

    constexpr const char* pci_root_bus_pnp_id = "PNP0A03";
    constexpr const char* pcie_root_bus_pnp_id = "PNP0A08";

    void init_pci();
    void parse_pci();

    const char* class_to_str(uint8_t class_code);
} // namespace x86_64::pci


#endif