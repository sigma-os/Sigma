#include <Sigma/arch/x86_64/drivers/mp.h>

x86_64::mp::mp::mp(){
    x86_64::mp::floating_pointer_table* pointer = this->find_pointer();
    if(pointer == nullptr){
        debug_printf("[MP]: Couldn't find floating pointer, add last kilobyte support\n");
        return;
    }

    this->floating_pointer = reinterpret_cast<uint64_t>(pointer);
    
    if(!this->check_floating_pointer()){
        printf("[MP]: Floating Pointer checksum failed\n");
        return;
    }

    debug_printf("[MP]: Found Floating Pointer stucture with revision: 1.%d, APIC Mode: ", pointer->revision);
    if(bitops<uint8_t>::bit_test(&(pointer->mp_feature_byte2), x86_64::mp::mp_feature_byte2_imcrp)) debug_printf("Virtual Wire\n");
    else debug_printf("PIC Mode\n");

    if(pointer->mp_feature_byte1 == 0){
        this->table = (pointer->physical_pointer_address + KERNEL_VBASE);

        if(memcmp(reinterpret_cast<const void*>(this->table), reinterpret_cast<const void*>(x86_64::mp::mp_configuration_table_header_signature), 4) != 0){
            printf("[MP]: Table header signature invalid\n");
            return;
        }

        if(!this->check_table_header()){
            printf("[MP]: Table header checksum failed\n");
            return;
        }

        x86_64::mp::configuration_table_header* header = reinterpret_cast<x86_64::mp::configuration_table_header*>(this->table);

        const char oem[9] = "";
        memcpy((void*)oem, (void*)&header->oem_id, 8);

        const char product[13] = "";
        memcpy((void*)product, (void*)&header->product_id, 12);

        debug_printf("[MP]: Found Table OEM ID: %s, Product ID: %s\n", oem, product);

        this->parse();
    } else {
        debug_printf("[MP]: Default Configuration: %d", pointer->mp_feature_byte1);
    }
}

bool x86_64::mp::mp::check_floating_pointer(){
    uint8_t* check_pointer = reinterpret_cast<uint8_t*>(this->floating_pointer);

    uint8_t sum = 0;

    for(size_t i = 0; i < sizeof(x86_64::mp::floating_pointer_table); i++) sum += check_pointer[i];

    return (sum == 0) ? (true) : (false);
}

bool x86_64::mp::mp::check_table_header(){
    uint8_t* check_pointer = reinterpret_cast<uint8_t*>(this->table);
    x86_64::mp::configuration_table_header* header = reinterpret_cast<x86_64::mp::configuration_table_header*>(check_pointer);

    uint8_t sum = 0;

    for(size_t i = 0; i < (header->base_table_length); i++) sum += check_pointer[i];

    return (sum == 0) ? (true) : (false);
}

x86_64::mp::floating_pointer_table* x86_64::mp::mp::find_pointer(){
    uint8_t* ebda_search = reinterpret_cast<uint8_t*>(x86_64::bios::bios::get_ebda_addr());
    uint8_t* ebda_search_end = reinterpret_cast<uint8_t*>(x86_64::bios::ebda_end);

    for(; (uintptr_t)(ebda_search) < (uintptr_t)(ebda_search_end); ebda_search += 16){
        if(memcmp(reinterpret_cast<const void*>(ebda_search), reinterpret_cast<const void*>(x86_64::mp::mp_floating_header_signature), 4) == 0){
            debug_printf("[MP]: Found Floating Pointer in EBDA at address: %x\n", reinterpret_cast<uint64_t>(ebda_search));
            return reinterpret_cast<x86_64::mp::floating_pointer_table*>(ebda_search);
        }
    }


    uint8_t* rom_search = reinterpret_cast<uint8_t*>(x86_64::bios::rom_space_location);
    uint8_t* rom_search_end = reinterpret_cast<uint8_t*>(x86_64::bios::rom_space_end);

    for(; (uintptr_t)(rom_search) < (uintptr_t)(rom_search_end); rom_search += 16){
        if(memcmp(reinterpret_cast<const void*>(rom_search), reinterpret_cast<const void*>(x86_64::mp::mp_floating_header_signature), 4) == 0){
            debug_printf("[MP]: Found Floating Pointer in ROM Area at address: %x\n", reinterpret_cast<uint64_t>(rom_search));
            return reinterpret_cast<x86_64::mp::floating_pointer_table*>(rom_search);
        }
    }

    // Search in last kilobyte of base?

    return nullptr;
}

void x86_64::mp::mp::parse(){
    x86_64::mp::configuration_table_header* table_header = reinterpret_cast<x86_64::mp::configuration_table_header*>(this->table);

    uint8_t* entry = reinterpret_cast<uint8_t*>((this->table + sizeof(x86_64::mp::configuration_table_header)));

    for(uint16_t i = 0; i < table_header->entry_count; i++){
        if(*entry > 4){
            printf("[MP]: Invalid table type %x", *entry);
            return;
        } 

        x86_64::mp::configuration_table_entry_type type = x86_64::mp::entry_types[*entry];

        //debug_printf("[MP]: Found entry %d: Type: %x, Name: %s, Size: %d\n", i, type.type, type.name, type.size);

        switch (type.type)
        {
            case x86_64::mp::configuration_table_entry_type_processor:
                this->parse_cpu(reinterpret_cast<uint64_t>(entry));
                break;

            case x86_64::mp::configuration_table_entry_type_bus:
                this->parse_bus(reinterpret_cast<uint64_t>(entry));
                break;

            case x86_64::mp::configuration_table_entry_type_ioapic:
                this->parse_ioapic(reinterpret_cast<uint64_t>(entry));
                break;
            
            case x86_64::mp::configuration_table_entry_type_io_interrupt_assignment:
                this->parse_io_interrupt_entry(reinterpret_cast<uint64_t>(entry));
                break;

            case x86_64::mp::configuration_table_entry_type_local_interrupt_assignment:
                this->parse_local_interrupt_entry(reinterpret_cast<uint64_t>(entry));
                break;
        
            default:
                break;
        }

        entry += type.size;
    }

}

void x86_64::mp::mp::parse_bus(uint64_t pointer){
    auto* entry = reinterpret_cast<x86_64::mp::configuration_table_entry_bus*>(pointer);
    
    const char bus_name[7] = "";
    memcpy((void*)&bus_name, (void*)&entry->bus_type_string, 6);

    debug_printf("[MP]: Bus Entry: Bus name: %s, Bus ID: %d\n", bus_name, entry->bus_id);
}

void x86_64::mp::mp::parse_cpu(uint64_t pointer){
    auto* entry = reinterpret_cast<x86_64::mp::configuration_table_entry_processor*>(pointer);

    debug_printf("[MP]: CPU Entry: LAPIC ID: %d, Stepping: %x, Model: %x, Family: %x", entry->lapic_id, entry->cpu_signature_stepping, entry->cpu_signature_model, entry->cpu_signature_family);
    if(!bitops<uint8_t>::bit_test(&(entry->cpu_flags), 0)) debug_printf(", CPU is Unusable");
    if(bitops<uint8_t>::bit_test(&(entry->cpu_flags), 1)) debug_printf(", CPU is BSP"); 
    debug_printf("\n");
}

void x86_64::mp::mp::parse_ioapic(uint64_t pointer){
    auto* entry = reinterpret_cast<x86_64::mp::configuration_table_entry_ioapic*>(pointer);

    debug_printf("[MP]: I/O APIC Entry: I/O APIC ID: %x, I/O APIC Version: %d, I/O APIC BASE %x", entry->ioapic_id, entry->ioapic_version, entry->ioapic_base_addr);

    if(!bitops<uint8_t>::bit_test(&(entry->ioapic_flags), 0)) debug_printf(", I/O APIC is Unusable");
    debug_printf("\n");
}

void x86_64::mp::mp::parse_io_interrupt_entry(uint64_t pointer){
    auto* entry = reinterpret_cast<x86_64::mp::configuration_table_entry_io_interrupt_assignment*>(pointer);

    debug_printf("[MP]: I/O Interrupt Asignment: Destinantion I/O APIC ID: %x, Destinantion I/O APIC INTIN#: %x, Source Bus ID: %x, Source Bus IRQ: %d", entry->destination_ioapic_id, entry->destination_ioapic_intin, entry->source_bus_id, entry->source_bus_irq);

    if(entry->interrupt_type == x86_64::mp::configuration_table_entry_io_interrupt_assignment_interrupt_type_int) debug_printf(", Interrupt Type: Interrupt");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_io_interrupt_assignment_interrupt_type_nmi) debug_printf(", Interrupt Type: NMI");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_io_interrupt_assignment_interrupt_type_smi) debug_printf(", Interrupt Type: SMI");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_io_interrupt_assignment_interrupt_type_extint) debug_printf(", Interrupt Type: ExtINT");
    else debug_printf(" Interrupt Type: Reserved");

    if(entry->polarity == x86_64::mp::configuration_table_entry_io_interrupt_assignment_polarity_conforming) debug_printf(", Polarity: Conforming to bus");
    else if(entry->polarity == x86_64::mp::configuration_table_entry_io_interrupt_assignment_polarity_active_high) debug_printf(", Polarity: Active high");
    else if(entry->polarity == x86_64::mp::configuration_table_entry_io_interrupt_assignment_polarity_active_low) debug_printf(", Polarity: Active low");
    else debug_printf(" Polarity: Reserved");

    if(entry->trigger_mode == x86_64::mp::configuration_table_entry_io_interrupt_assignment_trigger_mode_conforming) debug_printf(", Trigger mode: Conforming to bus");
    else if(entry->trigger_mode == x86_64::mp::configuration_table_entry_io_interrupt_assignment_trigger_mode_edge_triggered) debug_printf(", Trigger mode: Edge triggered");
    else if(entry->trigger_mode == x86_64::mp::configuration_table_entry_io_interrupt_assignment_trigger_mode_level_triggered) debug_printf(", Trigger mode: Leved triggered");
    else debug_printf(" Trigger mode: Reserved");

    debug_printf("\n");
}


void x86_64::mp::mp::parse_local_interrupt_entry(uint64_t pointer){
        auto* entry = reinterpret_cast<x86_64::mp::configuration_table_entry_local_interrupt_assignment*>(pointer);

    debug_printf("[MP]: Local Interrupt Asignment: Destinantion LAPIC ID: %x, Destinantion LAPIC LINTIN#: %x, Source Bus ID: %x, Source Bus IRQ: %d", entry->destination_lapic_id, entry->destination_lapic_lintin, entry->source_bus_id, entry->source_bus_irq);

    if(entry->interrupt_type == x86_64::mp::configuration_table_entry_local_interrupt_assignment_interrupt_type_int) debug_printf(", Interrupt Type: Interrupt");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_local_interrupt_assignment_interrupt_type_nmi) debug_printf(", Interrupt Type: NMI");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_local_interrupt_assignment_interrupt_type_smi) debug_printf(", Interrupt Type: SMI");
    else if(entry->interrupt_type == x86_64::mp::configuration_table_entry_local_interrupt_assignment_interrupt_type_extint) debug_printf(", Interrupt Type: ExtINT");
    else debug_printf(" Interrupt Type: Reserved");

    if(entry->polarity == x86_64::mp::configuration_table_entry_local_interrupt_assignment_polarity_conforming) debug_printf(", Polarity: Conforming to bus");
    else if(entry->polarity == x86_64::mp::configuration_table_entry_local_interrupt_assignment_polarity_active_high) debug_printf(", Polarity: Active high");
    else if(entry->polarity == x86_64::mp::configuration_table_entry_local_interrupt_assignment_polarity_active_low) debug_printf(", Polarity: Active low");
    else debug_printf(" Polarity: Reserved");

    if(entry->trigger_mode == x86_64::mp::configuration_table_entry_local_interrupt_assignment_trigger_mode_conforming) debug_printf(", Trigger mode: Conforming to bus");
    else if(entry->trigger_mode == x86_64::mp::configuration_table_entry_local_interrupt_assignment_trigger_mode_edge_triggered) debug_printf(", Trigger mode: Edge triggered");
    else if(entry->trigger_mode == x86_64::mp::configuration_table_entry_local_interrupt_assignment_trigger_mode_level_triggered) debug_printf(", Trigger mode: Leved triggered");
    else debug_printf(" Trigger mode: Reserved");

    debug_printf("\n");
}