#ifndef SIGMA_KERNEL_ACPI
#define SIGMA_KERNEL_ACPI

#include <Sigma/common.h>
#include <Sigma/boot_protocol.h>
#include <Sigma/mm/vmm.h>
#include <Sigma/acpi/tables.h>
#include <Sigma/acpi/fadt.h>
#include <klibc/stdio.h>
#include <Sigma/types/linked_list.h>

namespace acpi
{
    struct table {
        acpi::sdt_header header;
        uint8_t data[1];
    };

    void init(boot::boot_protocol* boot_protocol);

    // Physical! Address
    acpi::table* get_table(const char* signature);
    acpi::table* get_table(const char* signature, uint64_t index);

    uint16_t get_arch_boot_flags();
} // namespace acpi


#endif