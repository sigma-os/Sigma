#include <Sigma/acpi/acpi.h>

auto acpi_tables = types::linked_list<uint64_t>();

x86_64::paging::paging* previous_subsystem;
auto subsystem = x86_64::paging::paging();

static void enter_subsystem(){
    previous_subsystem = x86_64::paging::get_current_info();
    subsystem.set_paging_info();
}

static void leave_subsystem(){
    x86_64::paging::set_current_info(previous_subsystem);
}

static bool do_checksum(acpi::sdt_header* header){
    uint8_t sum = 0;

    for(size_t i = 0; i < header->length; i++) sum += ((uint8_t*)header)[i];

    return (sum == 0) ? (true) : (false);
}

acpi::table* acpi::get_table(const char* signature){
    enter_subsystem();
    for(auto table : acpi_tables){
        acpi::sdt_header* header = reinterpret_cast<acpi::sdt_header*>(table);

        if((signature[0] == header->signature[0]) && (signature[1] == header->signature[1]) && (signature[2] == header->signature[2]) && (signature[3] == header->signature[3])){
            leave_subsystem();
            return reinterpret_cast<acpi::table*>(reinterpret_cast<uint64_t>(header) - KERNEL_VBASE);
        }
    }

    debug_printf("[ACPI]: Couldn't find table %c%c%c%c\n", signature[0], signature[1], signature[2], signature[3]);
    leave_subsystem();
    return nullptr;
}

uint16_t acpi::get_arch_boot_flags(){
    acpi::fadt* fadt = reinterpret_cast<acpi::fadt*>(reinterpret_cast<uint64_t>(acpi::get_table(acpi::fadt_signature)) + KERNEL_VBASE);

    enter_subsystem();
    uint16_t flags = 0;

    #ifdef ARCH_X86_64
    flags = fadt->iapc_boot_arch;
    #elif ARCH_ARM
    flags = fadt->arm_boot_arch;
    #endif

    leave_subsystem();
    return flags;
}

void acpi::init(multiboot& mbd){
    mm::vmm::kernel_vmm::get_instance().clone_info(subsystem);

    enter_subsystem();

    acpi::rsdp* rsdp = reinterpret_cast<acpi::rsdp*>(mbd.get_rsdp());

    subsystem.map_page(reinterpret_cast<uint64_t>(rsdp), (reinterpret_cast<uint64_t>(rsdp) + KERNEL_VBASE), map_page_flags_present | map_page_flags_cache_disable | map_page_flags_no_execute);

    uint8_t rsdp_checksum = 0;
    for(size_t i = 0; i > sizeof(acpi::rsdp); i++) rsdp_checksum += ((uint8_t*)rsdp)[i];

    if(rsdp_checksum != 0){
        printf("[ACPI]: Failed RSDP checksum\n");
        leave_subsystem();
        return;
    }

    if(rsdp->revision > 0){
        debug_printf("[ACPI]: Detected version 2 or higher\n");
        acpi::xsdp* xsdp = reinterpret_cast<acpi::xsdp*>(rsdp);

        uint8_t xsdp_checksum = 0;
        for(size_t i = 0; i > sizeof(acpi::xsdp); i++) xsdp_checksum += ((uint8_t*)xsdp)[i];

        if(xsdp_checksum != 0){
            printf("[ACPI]: Failed XSDP checksum\n");
            leave_subsystem();
            return;
        }

        debug_printf("[ACPI]: Found XSDP: oem_id:%c%c%c%c%c%c, Revision: %d\n", xsdp->oem_id[0], xsdp->oem_id[1], xsdp->oem_id[2], xsdp->oem_id[3], xsdp->oem_id[4], xsdp->oem_id[5], xsdp->revision);
        

        subsystem.map_page(xsdp->xsdt_address, (xsdp->xsdt_address + KERNEL_VBASE), map_page_flags_present | map_page_flags_cache_disable | map_page_flags_no_execute);
        auto* xsdt = reinterpret_cast<acpi::rsdt*>(xsdp->xsdt_address + KERNEL_VBASE);

        if(!do_checksum(reinterpret_cast<acpi::sdt_header*>(xsdt))){
            printf("[ACPI]: Failed XSDT checksum\n");
            leave_subsystem();
            return;
        }

        size_t entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;
 
        for (size_t i = 0; i < entries; i++)
        {
            if(reinterpret_cast<uint64_t*>(xsdt->tables[i]) == nullptr) continue;
            subsystem.map_page(xsdt->tables[i], (xsdt->tables[i] + KERNEL_VBASE), map_page_flags_present | map_page_flags_cache_disable | map_page_flags_no_execute);
            auto* h = reinterpret_cast<acpi::sdt_header*>(xsdt->tables[i] + KERNEL_VBASE);
            if(do_checksum(h)){
                debug_printf("[ACPI]: Found Table %c%c%c%c: oem_id:%c%c%c%c%c%c, Revision: %d\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3], h->oem_id[0], h->oem_id[1], h->oem_id[2], h->oem_id[3], h->oem_id[4], h->oem_id[5], h->revision);

                acpi_tables.push_back(reinterpret_cast<uint64_t>(h));
            }
        }


    } else {
        debug_printf("[ACPI]: Found RSDP: oem_id:%c%c%c%c%c%c, Revision: %d\n", rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5], rsdp->revision);

        

        subsystem.map_page(rsdp->rsdt_address, (rsdp->rsdt_address + KERNEL_VBASE), map_page_flags_present | map_page_flags_cache_disable | map_page_flags_no_execute);

        auto* rsdt = reinterpret_cast<acpi::rsdt*>(rsdp->rsdt_address + KERNEL_VBASE);

        if(!do_checksum(reinterpret_cast<acpi::sdt_header*>(rsdt))){
            leave_subsystem();
            printf("[ACPI]: Failed RSDT checksum\n");
            return;
        }

        size_t entries = (rsdt->header.length - sizeof(acpi::sdt_header)) / 4;
        for (size_t i = 0; i < entries; i++)
        {
            if(reinterpret_cast<uint64_t*>(rsdt->tables[i]) == nullptr) continue;
            subsystem.map_page(rsdt->tables[i], (rsdt->tables[i] + KERNEL_VBASE), map_page_flags_present | map_page_flags_cache_disable | map_page_flags_no_execute);
            auto* h = reinterpret_cast<acpi::sdt_header*>(rsdt->tables[i] + KERNEL_VBASE);
            if(do_checksum(h)){
                //debug_printf("[ACPI]: Found table: %c%c%c%c\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3]);
                debug_printf("[ACPI]: Found Table %c%c%c%c: oem_id:%c%c%c%c%c%c, Revision: %d\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3], h->oem_id[0], h->oem_id[1], h->oem_id[2], h->oem_id[3], h->oem_id[4], h->oem_id[5], h->revision);

                acpi_tables.push_back(reinterpret_cast<uint64_t>(h));
            }

        }

    }

    leave_subsystem();
}