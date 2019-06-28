#ifndef SIGMA_KERNEL_BOOT_PROTOCOL
#define SIGMA_KERNEL_BOOT_PROTOCOL

#include <Sigma/common.h>

namespace boot
{
    struct boot_protocol {
        uint64_t acpi_pointer;
        uint64_t memsize;
        uint64_t mmap;
        uint64_t reserve_start; // Area to reserve
        uint64_t reserve_length; // Area to reserve
        uint64_t kernel_elf_sections;
        uint64_t kernel_n_elf_sections;
    };
} // namespace loader


#endif