#ifndef SIGMA_KERNEL_X86_64_PCI
#define SIGMA_KERNEL_X86_64_PCI

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/io.h>

namespace x86_64::pci
{
    constexpr uint16_t config_addr = 0xCF8;
    constexpr uint16_t config_data = 0xCFC;

    uint32_t read(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset);
    void write(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value);
} // namespace x86_64::pci


#endif