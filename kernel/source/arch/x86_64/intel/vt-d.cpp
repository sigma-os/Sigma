#include <Sigma/arch/x86_64/intel/vt-d.hpp>

#include <Sigma/arch/x86_64/drivers/pci.h>

#include <Sigma/generic/device.h>

static misc::lazy_initializer<x86_64::vt_d::iommu> global_iommu;


x86_64::vt_d::iommu& x86_64::vt_d::get_global_iommu(){
    if(!global_iommu)
        global_iommu.init();

    return *global_iommu;
}

#pragma region device_context_table

x86_64::vt_d::device_context_table::device_context_table(x86_64::vt_d::dma_remapping_engine* engine): engine{engine} {
    this->root_phys = (uint64_t)mm::pmm::alloc_block();
    mm::vmm::kernel_vmm::get_instance().map_page(this->root_phys, this->root_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);

    this->root = (root_table*)(this->root_phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    memset_aligned_4k((void*)this->root, 0);
}

x86_64::vt_d::device_context_table::~device_context_table(){
    // TODO: Free resources
}

x86_64::sl_paging::context& x86_64::vt_d::device_context_table::get_translation(uint8_t bus, uint8_t dev, uint8_t func){
    source_id sid{};
    sid.bus = bus;
    sid.dev = dev;
    sid.func = func;

    auto* root_entry = &this->root->entries[bus];
    if(!root_entry->present){
        uint64_t phys = (uint64_t)mm::pmm::alloc_block();
        uint64_t virt = phys + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE;

        mm::vmm::kernel_vmm::get_instance().map_page(phys, virt, map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute);
        memset_aligned_4k((void*)virt, 0);

        root_entry->context_table_ptr = (phys >> 12);
        root_entry->present = 1;
    }

    auto& context_table = *(vt_d::context_table*)((root_entry->context_table_ptr << 12) + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
    uint8_t context_table_index = sid.raw & 0xFF;
    auto* context_table_entry = &context_table.entries[context_table_index];

    if(!context_table_entry->present){
        context_table_entry->translation_type = 0; // Just use legacy translation
        // 3 level = 0b001, 4 level = 0b010 and so on, so n - 2 gives us the correct number
        context_table_entry->address_width = (this->engine->secondary_page_levels - 2);

        auto domain_id = this->engine->domain_ids.get_free_bit();
        ASSERT(domain_id != ~1ull);
        context_table_entry->domain_id = domain_id;

        auto* context = new sl_paging::context{this->engine->secondary_page_levels};
        this->sl_map.push_back(sid.raw, context);

        context_table_entry->second_level_page_translation_ptr = (context->get_ptr() >> 12);
        context_table_entry->present = 1;
    }

    return *this->sl_map[sid.raw];
}

uint64_t x86_64::vt_d::device_context_table::get_phys(){
    return this->root_phys;
}

#pragma endregion

#pragma region dma_remapping_engine

x86_64::vt_d::dma_remapping_engine::dma_remapping_engine(acpi_table::dma_remapping_def* def): pci_segment{def->segment_number} {
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

    debug_printf("         - ");
    if(((cap >> 8) & 0x1F) & (1 << 1)){
        debug_printf("3;");
        this->secondary_page_levels = 3;
    }
    if(((cap >> 8) & 0x1F) & (1 << 2)){
        debug_printf("4;");
        this->secondary_page_levels = 4;
    }
    if(((cap >> 8) & 0x1F) & (1 << 3)){
        debug_printf("5;");
        this->secondary_page_levels = 5;   
    }
    debug_printf(" Level Page Walks supported\n");
    ASSERT(this->secondary_page_levels != 0 && "[VT-d]: No secondary page levels supported");

    this->n_fault_recording_regs = ((cap >> 40) & 0xFF) + 1;
    debug_printf("         - Fault recording registers: %d\n", this->n_fault_recording_regs);
    this->fault_recording_regs = (fault_recording_reg*)(((uintptr_t)this->regs) + (16 * ((cap >> 24) & 0x3FF)));
    this->iotlb_regs = (iotlb_reg*)(((uintptr_t)this->regs) + (16 * ((extended_cap >> 8) & 0x3FF)));

    // (cap & 0x7) is a representation of the amount of bits of domain ids that the controller supports
    // 0b000 = 4 bits, 0b001 = 6bits, etc; So 2x + 4 will give us the correct amount of bits
    // then just pow2 that to find the total number
    this->n_domain_ids = 1 << (2 * (cap & 0x7) + 4);
    debug_printf("         - Number of Domain Ids: %d\n", this->n_domain_ids);

    this->domain_ids = types::bitmap{this->n_domain_ids};

    this->domain_ids.set(0); // Reserve domain id 0, is only really required if cap.CM is set, but not worth the hassle

    debug_printf("         Installing Root Table...");

    this->root_table = device_context_table{this};

    auto root_addr = dma_remapping_engine_regs::root_table_address_t{.raw = 0};
    root_addr.address = (this->root_table.get_phys() >> 12);
    root_addr.translation_type = 0; // Use legacy mode, we don't support scalable mode yet

    this->wbflush();
    this->regs->root_table_address = root_addr.raw;

    this->regs->global_command |= (1 << 30); // Ask hw to update root table ptr

    // Wait for HW to indicate root table ptr is initialized
    while(!(this->regs->global_status & (1 << 30)))
        ;

    this->invalidate_global_context();
    this->invalidate_iotlb();

    debug_printf("Done\n");

    asm("mfence" : : : "memory");

    debug_printf("         Enabling translation...");
    this->regs->global_command = (1 << 31);

    while(!(this->regs->global_status & (1 << 31)))
        ;

    debug_printf("Done\n");

    asm("mfence" : : : "memory");

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


    x86_64::idt::register_interrupt_handler({.vector = vector, .callback = +[](MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs, void* userptr){
        auto& self = *(dma_remapping_engine*)userptr;

        for(size_t i = 0; i < self.n_fault_recording_regs; i++){
            auto& fault = self.fault_recording_regs[i];

            if(fault.fault){
                printf("[VT-d]: Error at index %d occurred\n", i);
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

                source_id sid{.raw = (uint16_t)fault.sid};
                printf("        SID: %x, [seg: %d, bus: %d, dev: %d, func: %d]\n", fault.sid, self.def->segment_number, sid.bus, sid.dev, sid.func);

                if(fault.supervisor)
                    printf("        Supervisor access\n");
                else
                    printf("        User access\n");

                if(fault.execute)
                    printf("        Execute access\n");

                printf("        IO virtual address: %x\n", (fault.fault_info << 12));
            
                switch (fault.reason)
                {
                case 0x1:
                    printf("        Reason: The Present (P) field in root-entry used to process a request is 0.\n");
                    break;
                case 0x2:
                    printf("        Reason: The Present (P) field in context-entry used to process a request is 0.\n");
                    break;
                case 0x3:
                    printf("        Reason: Invalid programming of a context-entry used to process a request.\n");
                    break;
                case 0x5:
                    printf("        Reason: A Write or AtomicOp request encountered lack of write permission.\n");
                    break;
                case 0x6:
                    printf("        Reason: A Read or AtomicOp request encountered lack of read permission.\n");
                    break;
                case 0x7:
                    printf("        Reason: a hardware attempt to access a second-level paging entry (SL-PML4E, SL-PDPE, SL-PDE, or SL-PTE) referenced through the address (ADDR) field in a preceding second-level paging entry resulted in an error.\n");
                    break;
                case 0x8:
                    printf("        Reason: A hardware attempt to access a root-entry referenced through the Root-Table Address field in the Root-entry Table Address Register resulted in an error.\n");
                    break;
                case 0x9:
                    printf("        Reason: A hardware attempt to access a context-entry referenced through the CTP field in a root-entry resulted in an error.\n");
                    break;
                case 0xA:
                    printf("        Reason: Non-zero reserved field in a root-entry with Present (P) field set.\n");
                    break;
                default:
                    printf("        Unknown fault reason: %x\n", fault.reason);
                    break;
                }
            }
        }
        
        asm("cli; hlt");
    }, .userptr = (void*)this, .is_irq = true});
    
    // Clear IRQ Mask
    this->regs->fault_event_control &= ~(1 << 31);

    debug_printf("Done\n");
}

void x86_64::vt_d::dma_remapping_engine::wbflush(){
    if(this->regs->capabilites & (1 << 4)){ // Flushing supported
        this->regs->global_command = (1 << 27);

        while(this->regs->global_status & (1 << 27))
            ;
    }
}

void x86_64::vt_d::dma_remapping_engine::invalidate_global_context(){
    this->regs->context_command = (1ull << 63) | (1ull << 61); // Global invalidation

    // Wait for completion
    while(this->regs->context_command & (1ull << 63));
}

void x86_64::vt_d::dma_remapping_engine::invalidate_iotlb(){
    this->wbflush();
    iotlb_reg::iotlb_command cmd{};

    cmd.drain_reads = 1;
    cmd.drain_writes = 1;
    cmd.invalidate = 1;
    cmd.request_granularity = 1; // Global invalidation

    this->iotlb_regs->command = cmd.raw;

    // Wait for completion
    while(this->iotlb_regs->command & (1ull << 63))
        ;
}

#pragma endregion

#pragma region iommu

x86_64::vt_d::iommu::iommu(): active{false} {
    using namespace acpi_table;
    table = (dmar*)acpi::get_table("DMAR");
    if(!table)
        return;

    smp::cpu::get_current_cpu()->features.vt_d = 1;

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
        case reserved_mem_region_id: {
            auto* mem = (reserved_mem_region*)type;

            debug_printf("        - Reserved Memory Region\n");
            debug_printf("          PCI Segment Number: %d\n", mem->segment_number);
            debug_printf("          Region: %x -> %x\n", mem->region_base, mem->region_limit);
            break;
        }
        
        default:
            debug_printf("         - Unknown DMAR entry type: %d\n", *type);
            break;
        }

        offset += *(type + 1);
    }

    this->active = true;
}

x86_64::sl_paging::context& x86_64::vt_d::iommu::get_translation(uint16_t seg, uint8_t bus, uint8_t dev, uint8_t func){
    for(auto& entry : this->engines)
        if(entry.pci_segment == seg)
            return entry.root_table.get_translation(bus, dev, func);

    PANIC("Couldn't find iommu for segment");

    while(1)
        ;
}

#pragma endregion