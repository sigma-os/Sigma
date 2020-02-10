#include <Sigma/arch/x86_64/intel/vt-d.hpp>

#include <Sigma/arch/x86_64/drivers/pci.h>

#include <Sigma/generic/device.h>

#pragma region dma_remapping_engine

x86_64::vt_d::dma_remapping_engine::dma_remapping_engine(acpi_table::dma_remapping_def* def){
    this->def = def;

    mm::vmm::kernel_vmm::get_instance().map_page(def->register_base, def->register_base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);

    this->regs = (dma_remapping_engine_regs*)(def->register_base + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);

    uint8_t minor_ver = this->regs->version & 0xF;
    uint8_t major_ver = (this->regs->version >> 4) & 0xF;

    debug_printf("[VT-d]: Initializing DMA Remapping Engine, version %d.%d\n", major_ver, minor_ver);

    debug_printf("         Features:\n");

    auto cap = this->regs->capabilites;
    auto extended_cap = this->regs->extended_capabilities;
    if(cap & (1 << 3))
        debug_printf("         - Advanced Fault Logging\n");
    if(cap & (1 << 4))
        debug_printf("         - Write Buffer flushing required\n");
    if(cap & (1 << 5))
        debug_printf("         - Protected low memory region\n");
    if(cap & (1 << 6))
        debug_printf("         - Protected high memory region\n");

    if(extended_cap & (1ull << 43ull))
        debug_printf("         - Scalable translation mode\n");

    if(extended_cap & (1 << 4))
        debug_printf("         - 32bit APIC ids\n");
    else
        debug_printf("         - 8 bit APIC ids\n");

    this->n_fault_recording_regs = ((cap >> 40) & 0xFF) + 1;
    debug_printf("         - Fault recording registers: %d\n", this->n_fault_recording_regs);
    this->fault_recording_regs = (fault_recording_reg*)(((uintptr_t)this->regs) + (16 * ((cap >> 24) & 0x3FF)));

    this->root_phys = (uint64_t)mm::pmm::alloc_block();
    mm::vmm::kernel_vmm::get_instance().map_page(root_phys, root_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);
    this->root = (root_table*)(root_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    memset((void*)this->root, 0, 0x1000);

    debug_printf("         Installing Root Table...");

    auto root_addr = dma_remapping_engine_regs::root_table_address_t{.raw = 0};
    root_addr.address = this->root_phys;
    root_addr.translation_type = 0; // Use legacy mode, we don't support scalable mode yet
    this->regs->root_table_address = root_addr.raw;

    this->regs->global_command |= (1 << 30); // Ask hw to update root table ptr

    // Wait for HW to indicate root table ptr is initialized
    while(!(this->regs->global_status & (1 << 30)))
        ;

    debug_printf("Done\n");

    asm("" : : : "memory");

    //debug_printf("         Enabling translation...");
    //this->regs->global_command |= (1 << 31);

    //while(!(this->regs->global_status & (1 << 31)))
    //    ;

    //debug_printf("Done\n");

    asm("" : : : "memory");

    debug_printf("         Setting up and unmasking IRQ...");

    uint8_t vector = x86_64::idt::get_free_vector();

    // These are just PCI MSI regs, so steal structs from there, might want to make our own to ensure compat
    x86_64::pci::msi::address low_address{};
    x86_64::pci::msi::data data{};

    dma_remapping_engine_regs::fault_event_upper_address_t high_address{};


    uint32_t destination_id = 0; // Just send to BSP for now

    low_address.base_address = 0xFEE;
    low_address.destination_id = destination_id & 0xFF;

    data.delivery_mode = 0;
    data.vector = vector;

    if(smp::cpu::get_current_cpu()->features.x2apic)
        high_address.upper_destination_id = (destination_id >> 24) & 0xFFFFFF;

    this->regs->fault_event_data = data.raw;
    this->regs->fault_event_address = low_address.raw;
    this->regs->fault_event_upper_address = high_address.raw;


    x86_64::idt::register_interrupt_handler({.vector = vector, .callback = +[](x86_64::idt::idt_registers* regs, void* userptr){
        auto& self = *(dma_remapping_engine*)userptr;
        
        auto& fault = self.fault_recording_regs[0];

        printf("[VT-d]: Error occurred, RIP: %x\n", regs->rip);

        if(fault.fault){
        
            uint8_t type = fault.type_bit_1 | (fault.type_bit_2 << 1);
            switch (type)
            {
            case 0:
                printf("        Type: Write Request\n");
                break;
            case 1:
                printf("        Type: Read Request\n");
                break;
            case 2:
                printf("        Type: Page Request\n");
                break;
            case 3:
                printf("        Type: AtomicOp Request\n");
                break;
            default:
                break;
            }

            printf("        SID: %x, [%d:%d:%d:%d]\n", fault.sid, self.def->segment_number, (fault.sid >> 8) & 0xFF, (fault.sid >> 3) & 0x3F, fault.sid & 0x7);

            if(fault.supervisor)
                printf("        Supervisor access\n");
            else
                printf("        User access\n");

            if(fault.execute)
                printf("        Execute access\n");

            printf("        Fault address: %x\n", (fault.fault_info << 12));
            

            switch (fault.reason)
            {

            case 0x8:
                printf("[VT-d]: Root table entry access error");
                break;
            default:
                printf("[VT-d]: Unknown fault reason: %x\n", fault.reason);
                break;
            }

        }
        
        
        while(1)
            asm("pause");
    }, .userptr = (void*)this, .is_irq = true});
    
    this->regs->fault_event_control &= ~(1 << 31);

    debug_printf("Done\n");
}

#pragma endregion

#pragma region iommu

x86_64::vt_d::iommu::iommu(){
    using namespace acpi_table;
    table = (dmar*)acpi::get_table("DMAR");
    if(!table)
        return;

    debug_printf("[VT-d]: DMAR found\n");
    debug_printf("        - DMA Address Width: %d\n", table->host_dma_address_width + 1);

    if(table->flags.irq_remap)
        debug_printf("        - IRQ Remapping supported\n");
    if(table->flags.x2apic_opt_out)
        debug_printf("        - x2APIC opt out\n");
    if(table->flags.dma_control_opt_in)
        debug_printf("        - DMA Control Opt In\n");

    for(uint64_t offset = sizeof(dmar); offset < table->header.length;){
        uint16_t* type = (uint16_t*)((uintptr_t)table + offset);

        switch (*type)
        {
        case dma_remapping_def_id: {
            auto* remap = (dma_remapping_def*)type;
            debug_printf("        - DMA Remapping HW Definition\n");
            debug_printf("          PCI Segment number: %d\n", remap->segment_number);
            debug_printf("          Register base: %x\n", remap->register_base);
            debug_printf("          Flags\n");
            if(remap->flags.include_pci_all)
                debug_printf("             - Describes all PCI devices on segment\n");

            debug_printf("          Device scopes: \n");
            for(uint64_t device_offset = sizeof(dma_remapping_def); device_offset < remap->length;){
                auto* device = (device_scope*)((uintptr_t)type + device_offset);

                debug_printf("            - Type: %x\n", device->type);
                debug_printf("            - Enumeration Id: %x\n", device->enum_id);
                debug_printf("            - Start bus number: %x\n", device->start_bus_num);
                debug_printf("            - Path: ", device->start_bus_num);

                for(uint64_t path_offset = sizeof(device_scope); path_offset < device->length; path_offset += 2)
                    debug_printf("%x ", *(uint16_t*)((uintptr_t)device + path_offset));

                debug_printf("\n");

                device_offset += device->length;
            }

            this->engines.emplace_back(remap);

            break;
        }
        
        default:
            debug_printf("         - Unknown DMAR entry type: %d", *type);
            break;
        }

        offset += *(type + 1);
    }
}

#pragma endregion