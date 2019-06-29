#include <Sigma/arch/x86_64/drivers/pci.h>

static inline uint32_t make_pci_address(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset){
    return ((bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC) | (1u << 31));
}

uint32_t x86_64::pci::read(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset){

    x86_64::io::outd(x86_64::pci::config_addr, make_pci_address(bus, slot, function, offset));

    return x86_64::io::ind(x86_64::pci::config_data);
}

void x86_64::pci::write(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value){
    x86_64::io::outd(x86_64::pci::config_addr, make_pci_address(bus, slot, function, offset));

    x86_64::io::outd(x86_64::pci::config_data, value);
}