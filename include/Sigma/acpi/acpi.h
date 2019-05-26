#ifndef SIGMA_KERNEL_APCI
#define SIGMA_KERNEL_ACPI

#include <Sigma/common.h>
#include <Sigma/multiboot.h>
#include <Sigma/interfaces/paging_manager.h>
#include <Sigma/acpi/tables.h>

#include <Sigma/types/linked_list.h>

namespace acpi
{
    struct table {
        acpi::sdt_header header;
        uint8_t data[1];
    };

    void init(multiboot& mbd, IPaging& paging);

    // Physical! Address
    acpi::table* get_table(const char* signature);

} // namespace acpi


#endif