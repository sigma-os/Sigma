#ifndef SIGMA_KERNEL_ACPI_TABLES
#define SIGMA_KERNEL_ACPI_TABLES

#include <Sigma/common.h>

namespace acpi
{
    struct rsdp {
        char signature[8];
        uint8_t checksum;
        char oem_id[6];
        uint8_t revision;
        uint32_t rsdt_address;
    } __attribute__((packed));

    constexpr const char* rsdp_signature = "RSD PTR";

    struct xsdp {
        char signature[8];
        uint8_t checksum;
        char oem_id[6];
        uint8_t revision;
        uint32_t rsdt_address;
        uint32_t length;
        uint64_t xsdt_address;
        uint8_t extended_checksum;
        uint8_t reserved[3];
    } __attribute__((packed));

    struct sdt_header
    {
        char signature[4];
        uint32_t length;
        uint8_t revision;
        uint8_t checksum;
        char oem_id[6];
        char oem_tableid[8];
        uint32_t oem_revision;
        uint32_t creator_id;
        uint32_t creator_revision;
    } __attribute__((packed));

    constexpr uint64_t n_unique_acpi_tables = 28;

    struct rsdt {
        acpi::sdt_header header;
        uint32_t tables[n_unique_acpi_tables];
    } __attribute__((packed));

    struct xsdt {
        acpi::sdt_header header;
        uint64_t tables[n_unique_acpi_tables];
    } __attribute__((packed));    

    struct generic_address_structure {
        uint8_t id;
        uint8_t bit_width;
        uint8_t bit_offset;
        uint8_t access_size;
        uint64_t address;
    } __attribute__((packed));

    constexpr uint8_t generic_address_structure_id_system_memory = 0;
    constexpr uint8_t generic_address_structure_id_system_io = 1;
    constexpr uint8_t generic_address_structure_id_pci_configuration = 2;
    constexpr uint8_t generic_address_structure_id_embedded_controller = 3;
    constexpr uint8_t generic_address_structure_id_smbus = 4;
    constexpr uint8_t generic_address_structure_id_system_cmos = 5;
    constexpr uint8_t generic_address_structure_id_pci_bar_target = 6;
    constexpr uint8_t generic_address_structure_id_ipmi = 7;
    constexpr uint8_t generic_address_structure_id_general_pupose_io = 8;
    constexpr uint8_t generic_address_structure_id_generic_serial_bus = 9;
    constexpr uint8_t generic_address_structure_id_pcc = 10;
    constexpr uint8_t generic_address_structure_id_functional_fixed_hardware = 0x7F;

    constexpr uint8_t generic_address_structure_access_size_undefined = 0;
    constexpr uint8_t generic_address_structure_access_size_byte = 1; // 1 byte
    constexpr uint8_t generic_address_structure_access_size_word = 2; // 2 bytes
    constexpr uint8_t generic_address_structure_access_size_dword = 3; // 4 bytes
    constexpr uint8_t generic_address_structure_access_size_qword = 4; // 8 bytes

    constexpr const char* dsdt_signature = "DSDT";
} // namespace acpi



#endif
