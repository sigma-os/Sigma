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

    
    
    
} // namespace acpi



#endif
