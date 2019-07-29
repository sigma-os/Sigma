#include <loader/common.h>
#include <loader/multiboot.h>
#include <loader/protocol.h>

extern "C" {
    loader::boot_protocol boot_data;

    void _kernel_early();

    void kernel_loader_main(void* multiboot_information, uint64_t magic){
        loader::common::init();
    

        auto mboot = loader::multiboot(multiboot_information, magic);

    
    
        boot_data.acpi_pointer = mboot.get_rsdp();
        boot_data.kernel_elf_sections = reinterpret_cast<uint64_t>(mboot.get_elf_sections());
        boot_data.kernel_n_elf_sections = mboot.get_elf_n_sections();
        boot_data.memsize = mboot.get_memsize_mb();
        boot_data.mmap = reinterpret_cast<uint64_t>(mboot.get_mmap_entry());
        boot_data.reserve_start = mboot.get_mbd_ptr();
        boot_data.reserve_length = mboot.get_mbd_size();
        boot_data.kernel_initrd_ptr = mboot.get_initrd_ptr();
        boot_data.kernel_initrd_size = mboot.get_initrd_size();

        _kernel_early();
    }
}
