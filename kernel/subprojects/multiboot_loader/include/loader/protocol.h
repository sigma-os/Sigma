#ifndef SIGMA_LOADER_BOOT_PROTOCOL
#define SIGMA_LOADER_BOOT_PROTOCOL

#include <loader/common.h>

namespace loader
{
    struct boot_protocol {
        uint64_t acpi_pointer;
        uint64_t memsize;
        uint64_t mmap;
        uint64_t reserve_start; // Area to reserve
        uint64_t reserve_length; // Area to reserve
        uint64_t kernel_elf_sections;
        uint64_t kernel_n_elf_sections;
        uint64_t kernel_initrd_ptr;
        uint64_t kernel_initrd_size;
        char* cmdline;
    };
} // namespace loader


#endif