#ifndef SIGMA_KERNEL_X86_64_PCI
#define SIGMA_KERNEL_X86_64_PCI

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/io.h>
#include <Sigma/acpi/acpi.h>

namespace acpi
{
    constexpr const char* mcfg_table_signature = "MCFG";

    struct mcfg_table_entry {
        uint64_t base;
        uint16_t seg;
        uint8_t start_bus_number;
        uint8_t end_bus_number;
        uint32_t reserved;
    } __attribute__((packed));

    struct mcfg_table {
        acpi::sdt_header header;
        uint64_t reserved;
        mcfg_table_entry entries[1];
    } __attribute__((packed));
} // namespace acpi


namespace x86_64::pci
{
    constexpr uint16_t config_addr = 0xCF8;
    constexpr uint16_t config_data = 0xCFC;

    

    uint32_t read(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
    uint32_t read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
    void write(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value);
    void write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value);

    
    struct bar {
        uint8_t type;
        uint8_t number;
        uint8_t flags;
        uint64_t base;
        uint64_t len;
    };

    constexpr uint8_t bar_type_mem = 0;
    constexpr uint8_t bar_type_io = 0;
    constexpr uint8_t bar_type_invalid = 0xFF;

    constexpr uint8_t bar_flags_prefetchable = 0;

    x86_64::pci::bar read_bar(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint8_t number);
    x86_64::pci::bar read_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t number);

    struct device {
        bool exists;
        uint16_t seg;
        uint8_t bus, device, function;
        uint16_t vendor_id;
        uint8_t header_type, class_code, subclass_code;
        uint32_t gsi;
        x86_64::pci::bar bars[6];
    };

    using pci_iterator = uint64_t;

    x86_64::pci::device iterate(pci_iterator& iterator);

    void parse_pci();
} // namespace x86_64::pci


#endif