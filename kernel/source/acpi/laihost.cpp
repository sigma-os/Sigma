#include <klibc/stdio.h>
#include <klibc/stdlib.h>
#include <Sigma/acpi/acpi.h>
#include <Sigma/arch/x86_64/io.h>
#include <Sigma/arch/x86_64/drivers/pci.h>
#include <Sigma/arch/x86_64/drivers/hpet.h>
#include <Sigma/mm/vmm.h>

#include <lai/host.h>

extern "C" {

void *laihost_malloc(size_t sz){
    if(sz == 0) sz = 1; // Just in case
    return malloc(sz);
}

void *laihost_realloc(void *ptr, size_t size){
    return realloc(ptr, size);
}

void laihost_free(void *ptr){
    free(ptr);
}

void laihost_log(int level, const char *msg){
    switch (level)
    {
    case LAI_DEBUG_LOG:
        debug_printf("[LAI] Debug: %s\n", msg);
        break;

    case LAI_WARN_LOG:
        printf("[LAI] Warning: %s\n", msg);
        //debug_printf("[LAI] Warning: %s\n", msg);
        break;
    
    default:
        debug_printf("[LAI] Unknown log level: %s\n", msg);
        break;
    }
}

void laihost_panic(const char * msg){
    PANIC(msg);
    asm("cli; hlt");
    while(true);
}

void *laihost_scan(const char * signature, size_t index){
    return static_cast<void*>(acpi::get_table(signature, index));
}

void *laihost_map(size_t addr, size_t bytes){
    uint64_t n_pages = misc::div_ceil(bytes, mm::pmm::block_size); // Ceil it to make sure we have all bytes allocated in case (bytes & 0xFFF) != 0

    for(size_t i = 0; i < n_pages; i++){
        uint64_t offset = (i * mm::pmm::block_size);
        mm::vmm::kernel_vmm::get_instance().map_page((addr + offset), (addr + offset + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute | map_page_flags_global);
    }

    return reinterpret_cast<void*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
}

void laihost_unmap(void*, size_t){
    debug_printf("[LAI]: Ignoring laihost_unmap()\n");
}

void laihost_outb(uint16_t port, uint8_t val){
    x86_64::io::outb(port, val);
}

void laihost_outw(uint16_t port, uint16_t val){
    x86_64::io::outw(port, val);
}

void laihost_outd(uint16_t port, uint32_t val){
    x86_64::io::outd(port, val);
}

uint8_t laihost_inb(uint16_t port){
    return x86_64::io::inb(port);
}

uint16_t laihost_inw(uint16_t port){
    return x86_64::io::inw(port);
}

uint32_t laihost_ind(uint16_t port){
    return x86_64::io::ind(port);
}

void laihost_pci_writeb(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset , uint8_t value){
    x86_64::pci::write(seg, bus, device, function, offset, value, 1);
}

uint8_t laihost_pci_readb(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset){
    return x86_64::pci::read(seg, bus, device, function, offset, 1);
}

void laihost_pci_writew(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset , uint16_t value){
    x86_64::pci::write(seg, bus, device, function, offset, value, 2);
}

uint16_t laihost_pci_readw(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset){
    return x86_64::pci::read(seg, bus, device, function, offset, 2);
}

void laihost_pci_writed(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset , uint32_t value){
    x86_64::pci::write(seg, bus, device, function, offset, value, 4);
}

uint32_t laihost_pci_readd(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset){
    return x86_64::pci::read(seg, bus, device, function, offset, 4);
}

void laihost_sleep(uint64_t ms){
    x86_64::hpet::poll_sleep(ms);
}

}