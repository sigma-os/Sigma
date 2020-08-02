#include <Sigma/acpi/acpi.h>
#include <Sigma/proc/initrd.h>

#include <lai/core.h>
#include <lai/helpers/sci.h>
#include <lai/helpers/pm.h>
#include <lai/drivers/ec.h>
#include <lai/helpers/pc-bios.h>

static uint64_t revision = 0;
static auto acpi_tables = types::linked_list<uint64_t>();
static acpi::table* dsdt_override{};

static bool do_checksum(acpi::sdt_header* header){
    uint8_t sum = 0;

    for(size_t i = 0; i < header->length; i++) sum += ((uint8_t*)header)[i];

    return (sum == 0);
}

acpi::table* acpi::get_table(const char* signature, uint64_t index) {
	debug_printf("[ACPI]: Requesting table: %s, index: %d...", signature, index);
	if(memcmp(signature, acpi::dsdt_signature, 4) == 0) {
        uint64_t dsdt_addr = 0;
        if(auto* override = misc::kernel_args::get_str("dsdt_override"); override != nullptr){
            // DSDT has been overridden via kernel arguments
            debug_printf("loading DSDT via override at %s, ", override);
            dsdt_addr = reinterpret_cast<uint64_t>(dsdt_override);
        } else {
            auto* fadt = reinterpret_cast<acpi::fadt*>(acpi::get_table(acpi::fadt_signature));
		    uint64_t dsdt_phys_addr = 0;

		    if(misc::is_canonical(fadt->x_dsdt) && revision != 1)
			    dsdt_phys_addr = fadt->x_dsdt;
		    else
			    dsdt_phys_addr = fadt->dsdt;

		    dsdt_addr = (dsdt_phys_addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

		    mm::vmm::kernel_vmm::get_instance().map_page(dsdt_phys_addr, dsdt_addr,
			    										 map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
		    for(size_t i = 1; i < ((((acpi::sdt_header*)dsdt_addr)->length / mm::pmm::block_size) + 1); i++) {
			    mm::vmm::kernel_vmm::get_instance().map_page(
                        (dsdt_phys_addr + (mm::pmm::block_size * i)), (dsdt_addr + (mm::pmm::block_size * i)),
				    map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
		    }

            debug_printf("Found at: %x\n", dsdt_addr);
        }
		return reinterpret_cast<acpi::table*>(dsdt_addr);
	}

	uint64_t curr = 0;
	for(const auto table : acpi_tables) {
		auto* header = reinterpret_cast<acpi::sdt_header*>(table);

		if((signature[0] == header->signature[0]) && (signature[1] == header->signature[1]) &&
		   (signature[2] == header->signature[2]) && (signature[3] == header->signature[3])) {
			if(curr != index)
				curr++;
			else {
				debug_printf("Found at: %x\n", header);
				return reinterpret_cast<acpi::table*>(header);
			}
		}
	}

	debug_printf("Didn't find\n");
	return nullptr;
}

acpi::table* acpi::get_table(const char* signature){
    if(memcmp(signature, acpi::dsdt_signature, 4) == 0) {
        uint64_t dsdt_addr = 0;
        if(auto* override = misc::kernel_args::get_str("dsdt_override"); override != nullptr){
            // DSDT has been overridden via kernel arguments
            debug_printf("[ACPI]: Loading DSDT via override at %s\n", override);
            dsdt_addr = reinterpret_cast<uint64_t>(dsdt_override);
        } else {
            auto* fadt = reinterpret_cast<acpi::fadt*>(acpi::get_table(acpi::fadt_signature));
		    uint64_t dsdt_phys_addr = 0;

		    if(misc::is_canonical(fadt->x_dsdt) && revision != 0)
			    dsdt_phys_addr = fadt->x_dsdt;
		    else
			    dsdt_phys_addr = fadt->dsdt;

		    dsdt_addr = (dsdt_phys_addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

		    mm::vmm::kernel_vmm::get_instance().map_page(dsdt_phys_addr, dsdt_addr,
			    										 map_page_flags_present |
				    										 map_page_flags_no_execute, map_page_cache_types::uncacheable);
		    for(size_t i = 1; i < ((((acpi::sdt_header*)dsdt_addr)->length / mm::pmm::block_size) + 1); i++) {
			    mm::vmm::kernel_vmm::get_instance().map_page(
                        (dsdt_phys_addr + (mm::pmm::block_size * i)), (dsdt_addr + (mm::pmm::block_size * i)),
				    map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
		    }

            debug_printf("Found at: %x\n", dsdt_addr);
        }
		
		
		return reinterpret_cast<acpi::table*>(dsdt_addr);
	}
    for(auto table : acpi_tables){
        auto* header = reinterpret_cast<acpi::sdt_header*>(table);

        if((signature[0] == header->signature[0]) && (signature[1] == header->signature[1]) && (signature[2] == header->signature[2]) && (signature[3] == header->signature[3])){
            return reinterpret_cast<acpi::table*>(header);
        }
    }

    debug_printf("[ACPI]: Couldn't find table %c%c%c%c\n", signature[0], signature[1], signature[2], signature[3]);
    return nullptr;
}

uint16_t acpi::get_arch_boot_flags(){
    auto* fadt = reinterpret_cast<acpi::fadt*>(acpi::get_table(acpi::fadt_signature));

    #if defined(ARCH_X86_64)
    uint16_t flags = fadt->iapc_boot_arch;
    #elif defined(ARCH_ARM)
    uint16_t flags = fadt->arm_boot_arch;
    #endif

    return flags;
}

void acpi::init(MAYBE_UNUSED_ATTRIBUTE boot::boot_protocol* boot_protocol){
    FUNCTION_CALL_ONCE();
    
    lai_rsdp_info info{};
    if(auto error = lai_bios_detect_rsdp(&info); error != LAI_ERROR_NONE){
        printf("[ACPI]: Failed finding {R, X}SDP, Error: %s\n", lai_api_error_to_string(error));
        return;
    }

    mm::vmm::kernel_vmm::get_instance().map_page(info.rsdp_address, info.rsdp_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::normal);
    auto* rsdp = reinterpret_cast<acpi::rsdp*>(info.rsdp_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    revision = info.acpi_version;

    if(revision > 1){
        debug_printf("[ACPI]: Detected version 2 or higher\n");
        auto* xsdp = reinterpret_cast<acpi::xsdp*>(rsdp);

        debug_printf("[ACPI]: Found XSDP: oem_id: \"%c%c%c%c%c%\", Revision: %d\n", xsdp->oem_id[0], xsdp->oem_id[1], xsdp->oem_id[2], xsdp->oem_id[3], xsdp->oem_id[4], xsdp->oem_id[5], xsdp->revision);

        mm::vmm::kernel_vmm::get_instance().map_page(xsdp->xsdt_address, (xsdp->xsdt_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);

        auto* xsdt = reinterpret_cast<acpi::xsdt*>(xsdp->xsdt_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

        if(!do_checksum(reinterpret_cast<acpi::sdt_header*>(xsdt))){
            printf("[ACPI]: Failed XSDT checksum\n");
            return;
        }

        size_t entries = (xsdt->header.length - sizeof(xsdt->header)) / 8;

        for (size_t i = 0; i < entries; i++)
        {
            if(reinterpret_cast<uint64_t*>(xsdt->tables[i]) == nullptr) continue;
            mm::vmm::kernel_vmm::get_instance().map_page(xsdt->tables[i], (xsdt->tables[i] + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
            auto* h = reinterpret_cast<acpi::sdt_header*>(xsdt->tables[i] + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

            uint64_t addr = xsdt->tables[i] - mm::pmm::block_size;
            auto n_pages = misc::div_ceil(h->length, mm::pmm::block_size) + 2;
            for(uint64_t j = 0; j < n_pages; j++){
                uint64_t phys = addr + (j * mm::pmm::block_size);
                uint64_t virt = phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
                mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
            }


            if(do_checksum(h)){
                debug_printf("[ACPI]: Found Table %c%c%c%c: oem_id: \"%c%c%c%c%c%c\", Revision: %d\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3], h->oem_id[0], h->oem_id[1], h->oem_id[2], h->oem_id[3], h->oem_id[4], h->oem_id[5], h->revision);
                acpi_tables.push_back(reinterpret_cast<uint64_t>(h));
            }

        }
    } else {
        debug_printf("[ACPI]: Found RSDP: oem_id: \"%c%c%c%c%c%c\", Revision: %d\n", rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5], rsdp->revision);
        mm::vmm::kernel_vmm::get_instance().map_page(info.rsdt_address, (info.rsdt_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
        auto* rsdt = reinterpret_cast<acpi::rsdt*>(info.rsdt_address + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
        if(!do_checksum(reinterpret_cast<acpi::sdt_header*>(rsdt))){
            printf("[ACPI]: Failed RSDT checksum\n");
            return;
        }

        size_t entries = (rsdt->header.length - sizeof(acpi::sdt_header)) / 4;
        for (size_t i = 0; i < entries; i++)
        {
            if(reinterpret_cast<uint64_t*>(rsdt->tables[i]) == nullptr) continue;
            auto* h = reinterpret_cast<acpi::sdt_header*>(rsdt->tables[i] + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
            mm::vmm::kernel_vmm::get_instance().map_page(rsdt->tables[i], (rsdt->tables[i] + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);

            uint64_t addr = rsdt->tables[i] - mm::pmm::block_size;
            auto n_pages = misc::div_ceil(h->length, mm::pmm::block_size) + 2;
            for(uint64_t j = 0; j < n_pages; j++){
                uint64_t phys = addr + (j * mm::pmm::block_size);
                uint64_t virt = phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;
                mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_no_execute, map_page_cache_types::uncacheable);
            }

            if(do_checksum(h)){
                debug_printf("[ACPI]: Found Table %c%c%c%c: oem_id: \"%c%c%c%c%c%c\", Revision: %d, Addr: %x\n", h->signature[0], h->signature[1], h->signature[2], h->signature[3], h->oem_id[0], h->oem_id[1], h->oem_id[2], h->oem_id[3], h->oem_id[4], h->oem_id[5], h->revision, h);

                acpi_tables.push_back(reinterpret_cast<uint64_t>(h));
            }
        }
    }

    if(auto* override = misc::kernel_args::get_str("dsdt_override"); override != nullptr){
        size_t size = proc::initrd::get_size(override);
        if(size == 0)
            PANIC("Supplied DSDT override size is 0");

        auto* buffer = new uint8_t[size];
        proc::initrd::read_file(override, buffer, 0, size);

        dsdt_override = reinterpret_cast<acpi::table*>(buffer);
    }

    if(misc::kernel_args::get_bool("acpi_trace"))
        lai_enable_tracing(LAI_TRACE_OP);

    lai_set_acpi_revision(rsdp->revision);

    lai_create_namespace();

    acpi::init_ec();
}

static void acpi_sci_handler(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs, MAYBE_UNUSED_ATTRIBUTE void* userptr) {
	uint16_t event = lai_get_sci_event();
	if(event & ACPI_POWER_BUTTON) {
		debug_printf("[ACPI]: Requested ACPI shutdown at TSC: %x\n", x86_64::read_tsc());
		lai_enter_sleep(5); // S5 is off
	} else {
		printf("[ACPI]: Unknown SCI event: %x\n", event);
	}
}

void acpi::init_sci(acpi::madt& madt){
    FUNCTION_CALL_ONCE();
    auto* fadt = reinterpret_cast<acpi::fadt*>(acpi::get_table(acpi::fadt_signature));
    uint16_t sci_int = fadt->sci_int;

    if(!madt.supports_legacy_pic()){
        // FADT->SCI_INT contains a GSI, map ourselves

        // ACPI spec states that is it a sharable, level, active low interrupt
        x86_64::apic::ioapic::set_entry(sci_int, (sci_int + 0x20), x86_64::apic::ioapic_delivery_modes::FIXED, x86_64::apic::ioapic_destination_modes::PHYSICAL, ((1 << 13) | (1 << 15)), smp::cpu::get_current_cpu()->lapic_id); // Target the BSP
    }

    x86_64::idt::register_interrupt_handler({.vector = (uint16_t)(sci_int + 0x20), .callback = acpi_sci_handler, .is_irq = true});
    x86_64::apic::ioapic::unmask_irq(sci_int);

    lai_enable_acpi(1); // argument is interrupt mode, 1 = APIC, 0 = PIC, 2 = SAPIC, SAPIC doesn't even exist on x86_64 only on IA64(Itanium)    

    debug_printf("[ACPI]: Enabled SCI on IRQ: %x\n", sci_int);
}

void acpi::init_ec(){
    FUNCTION_CALL_ONCE();
    LAI_CLEANUP_STATE lai_state_t state;
    lai_init_state(&state);
 
    LAI_CLEANUP_VAR lai_variable_t pnp_id = {};
    lai_eisaid(&pnp_id, ACPI_EC_PNP_ID);
 
    struct lai_ns_iterator it = {};
    lai_nsnode_t* node = nullptr;
    while((node = lai_ns_iterate(&it))){
        if(lai_check_device_pnp_id(node, &pnp_id, &state)) // This is not an EC
            continue;
 
        // Found one
        auto* driver = new lai_ec_driver; // Dynamically allocate the memory since we dont know how many ECs there could be
        lai_init_ec(node, driver);                               
 
        struct lai_ns_child_iterator child_it = LAI_NS_CHILD_ITERATOR_INITIALIZER(node);
        lai_nsnode_t *child_node;
        while((child_node = lai_ns_child_iterate(&child_it))){
            if(lai_ns_get_node_type(child_node) == LAI_NODETYPE_OPREGION){
                if(lai_ns_get_opregion_address_space(child_node) == ACPI_OPREGION_EC){
                    lai_ns_override_opregion(child_node, &lai_ec_opregion_override, driver);
                }
            }
        }

        printf("[ACPI]: Initializing Embedded Controller at %s [cmd: %x, data: %x] ...", lai_stringify_node_path(node), driver->cmd_port, driver->data_port);

        auto* reg = lai_resolve_path(node, "_REG");
        if(reg){
            LAI_CLEANUP_VAR lai_variable_t address_space = {};
            LAI_CLEANUP_VAR lai_variable_t enable = {};

            address_space.type = LAI_INTEGER;
            address_space.integer = acpi::generic_address_structure_id_embedded_controller;

            enable.type = LAI_INTEGER;
            enable.integer = 1; // Enable

            if(auto error = lai_eval_largs(nullptr, reg, &state, &address_space, &enable, nullptr); error != LAI_ERROR_NONE){
                printf("Failed to evaluate %s(EmbeddedControl, 1) due to error %s(%d)\n", lai_stringify_node_path(reg), lai_api_error_to_string(error), error);
                return;
            }
        }

        printf("Success\n");
    }
}