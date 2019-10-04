#include <Sigma/arch/x86_64/drivers/pci.h>

extern "C"{
    #include <lai/core.h>
}

static uint32_t legacy_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size);
static void legacy_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size);

using read_func = uint32_t (*)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint8_t);
using write_func = void (*)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint8_t);

read_func internal_read = legacy_read; // Default to legacy functions they should always work
write_func internal_write = legacy_write;

auto mcfg_entries = types::linked_list<acpi::mcfg_table_entry>();
auto pci_devices = types::linked_list<x86_64::pci::device>();

static inline uint32_t make_pci_address(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset){
    return ((bus << 16) | (slot << 11) | (function << 8) | (offset & 0xFC) | (1u << 31));
}

static uint32_t legacy_read(MAYBE_UNUSED_ATTRIBUTE uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, 
							uint16_t offset, uint8_t access_size) {
	x86_64::io::outd(x86_64::pci::config_addr, make_pci_address(bus, slot, function, offset));
	switch(access_size) {
		case 1: // Byte
			return x86_64::io::inb(x86_64::pci::config_data + (offset % 4));
			break;

		case 2: // Word
			return x86_64::io::inw(x86_64::pci::config_data + (offset % 4));
			break;

		case 4: // DWord
			return x86_64::io::ind(x86_64::pci::config_data + (offset % 4));
			break;

		default:
			printf("[PCI]: Unknown Access size [%d]\n", access_size);
			return 0;
			break;
	}
}

static void legacy_write(MAYBE_UNUSED_ATTRIBUTE uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, 
						uint16_t offset, uint32_t value, uint8_t access_size) {
	x86_64::io::outd(x86_64::pci::config_addr, make_pci_address(bus, slot, function, offset));
	switch(access_size) {
		case 1: // Byte
			x86_64::io::outb(x86_64::pci::config_data + (offset % 4), value);
			break;

		case 2: // Word
			x86_64::io::outw(x86_64::pci::config_data + (offset % 4), value);
			break;

		case 4: // DWord
			x86_64::io::outd(x86_64::pci::config_data + (offset % 4), value);
			break;

		default:
			printf("[PCI]: Unknown Access size [%d]\n", access_size);
			break;
	}
}

static uint32_t mcfg_pci_read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size){
    for(const auto& entry : mcfg_entries){
        if(entry.seg == seg){
            // Maybe?
            if((bus >= entry.start_bus_number) && (bus <= entry.end_bus_number)){
                // Found it

                uint64_t addr = (entry.base + (((bus - entry.start_bus_number) << 20) | (slot << 15) | (function << 12))) | (offset);
                mm::vmm::kernel_vmm::get_instance().map_page(addr, (addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute | map_page_flags_global | map_page_flags_cache_disable);
                switch (access_size)
                {
                case 1: // Byte
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint8_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        return *ptr;
                    }
                    break;
                
                case 2: // Word
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint16_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        return *ptr;
                    }
                    break;

                case 4: // DWord
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint32_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        return *ptr;
                    }
                    break;


                default:
                    printf("[PCI]: Unknown Access size [%d]\n", access_size);
                    break;
                }
            }
        }
    }

    debug_printf("[PCI]: Tried to read from nonexistent device, %x:%x:%x:%x\n", seg, bus, slot, function);
    return 0;
}

static void mcfg_pci_write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size){
    for(const auto& entry : mcfg_entries){
        if(entry.seg == seg){
            // Maybe?
            if((bus >= entry.start_bus_number) && (bus <= entry.end_bus_number)){
                // Found it

                uint64_t addr = (entry.base + (((bus - entry.start_bus_number) << 20) | (slot << 15) | (function << 12))) + offset;
                mm::vmm::kernel_vmm::get_instance().map_page(addr, (addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE), map_page_flags_present | map_page_flags_writable | map_page_flags_no_execute | map_page_flags_global | map_page_flags_cache_disable);
                switch (access_size)
                {
                case 1: // Byte
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint8_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        *ptr = (uint8_t)value;
                    }
                    break;
                
                case 2: // Word
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint16_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        *ptr = (uint16_t)value;
                    }
                    break;

                case 4: // DWord
                    {
                        volatile auto* ptr = reinterpret_cast<volatile uint32_t*>(addr + KERNEL_PHYSICAL_VIRTUAL_MAPPING_BASE);
                        *ptr = value;
                    }
                    break;


                default:
                    printf("[PCI]: Unknown Access size [%d]\n", access_size);
                    break;
                }
                return;
            }
        }
    }

    debug_printf("[PCI]: Tried to write to nonexistent device, %x:%x:%x:%x\n", seg, bus, slot, function);
}

static const char* class_to_str(uint8_t class_code){
    switch (class_code)
    {
    case 0:
        return "Undefined";
        break;

    case 1:
        return "Mass Storage Controller";
        break;
    
    case 2:
        return "Network Controller";
        break;
    
    case 3:
        return "Display Controller";
        break;
    
    case 4:
        return "Multimedia controller";
        break;

    case 5:
        return "Memory Controller";
        break;

    case 6:
        return "Bridge Device";
        break;

    case 0xC:
        return "Serial Bus Controller";
        break;
    
    default:
        return "Unknown";
        break;
    }
}

static void enumerate_bus(uint16_t seg, uint8_t bus);

static void enumerate_function(uint16_t seg, uint8_t bus, uint8_t device, uint8_t function){
    auto dev = x86_64::pci::device();
    uint16_t vendor_id = x86_64::pci::read(seg, bus, device, function, 0, 2);
    if(vendor_id == 0xFFFF) return; // Device doesn't exist

    dev.exists = true;
    dev.seg = seg;
    dev.bus = bus;
    dev.device = device;
    dev.function = function;
    dev.vendor_id = vendor_id;

    uint8_t class_code = ((x86_64::pci::read(seg, bus, device, function, 8, 4) >> 24) & 0xFF);
    uint8_t subclass_code = ((x86_64::pci::read(seg, bus, device, function, 8, 4) >> 16) & 0xFF);
    if(class_code == 0x6 && subclass_code == 0x4){
        // PCI to PCI bridge
        uint8_t secondary_bus = x86_64::pci::read(seg, bus, device, function, 0x19, 1);
        enumerate_bus(seg, secondary_bus);
    }

    dev.class_code = class_code;
    dev.subclass_code = subclass_code;

    uint8_t header_type = ((x86_64::pci::read(seg, bus, device, 0, 0xC, 4) >> 16) & 0xFF);
    header_type &= 0x7F; // Ignore Multifunction bit
    dev.header_type = header_type;
    if(header_type == 0){
        // Normal device has 5 bars
        for(uint8_t i = 0; i < 6; i++) dev.bars[i] = x86_64::pci::read_bar(seg, bus, device, function, i);
    } else if(header_type == 1){
        // PCI to PCI bridge has 2 bars
        for(uint8_t i = 0; i < 3; i++) dev.bars[i] = x86_64::pci::read_bar(seg, bus, device, function, i);
    }

    // TODO: route IRQs

    pci_devices.push_back(dev);
}

static void enumerate_device(uint16_t seg, uint8_t bus, uint8_t device){
    uint16_t vendor_id = x86_64::pci::read(seg, bus, device, 0, 0, 2);
    if(vendor_id == 0xFFFF) return; // Device doesn't exist

    uint8_t header_type = x86_64::pci::read(seg, bus, device, 0, 0xE, 1);
    if(bitops<uint8_t>::bit_test(header_type, 7)){
        for(uint8_t i = 0; i < 8; i++) enumerate_function(seg, bus, device, i);
    } else {
        enumerate_function(seg, bus, device, 0);
    }
}

static void enumerate_bus(uint16_t seg, uint8_t bus){
    for(uint8_t i = 0; i < 32; i++) enumerate_device(seg, bus, i);
}

void x86_64::pci::parse_pci(){
    auto* mcfg = reinterpret_cast<acpi::mcfg_table*>(acpi::get_table(acpi::mcfg_table_signature));
    if(mcfg == nullptr){
        // No PCI-E use legacy access
        internal_read = legacy_read;
        internal_write = legacy_write;

        uint8_t header_type = x86_64::pci::read(0, 0, 0, 0, 0xE, 1);
        if(bitops<uint8_t>::bit_test(header_type, 7)){
            for(uint8_t i = 0; i < 8; i++){
                uint16_t vendor_id = x86_64::pci::read(0, 0, 0, i, 0, 2);
                if(vendor_id == 0xFFFF) break; // Device doesn't exist
                enumerate_bus(0, i);
            }
        } else {
            enumerate_bus(0, 0);
        }
    } else {
        size_t n_entries = (mcfg->header.length - (sizeof(acpi::sdt_header) + sizeof(uint64_t))) / sizeof(acpi::mcfg_table_entry); 
        for(uint64_t i = 0; i < n_entries; i++){
            acpi::mcfg_table_entry& entry = mcfg->entries[i];
            mcfg_entries.push_back(entry);
        }


        internal_read = mcfg_pci_read;
        internal_write = mcfg_pci_write;

        LAI_CLEANUP_STATE lai_state_t state;
        lai_init_state(&state);

        lai_variable_t pci_pnp_id = {};
        lai_variable_t pcie_pnp_id = {};
        lai_eisaid(&pci_pnp_id, const_cast<char*>(x86_64::pci::pci_root_bus_pnp_id));
        lai_eisaid(&pcie_pnp_id, const_cast<char*>(x86_64::pci::pcie_root_bus_pnp_id));

        struct lai_ns_iterator iter = LAI_NS_ITERATOR_INITIALIZER;
        lai_nsnode_t *node;
        while ((node = lai_ns_iterate(&iter))) {
            if (lai_check_device_pnp_id(node, &pci_pnp_id, &state) &&
                    lai_check_device_pnp_id(node, &pcie_pnp_id, &state)) {
                continue;
            }

            lai_variable_t bus_number = {};
            uint64_t bbn_result = 0;
            lai_nsnode_t *bbn_handle = lai_resolve_path(node, "_BBN");
            if (bbn_handle) {
                if (lai_eval(&bus_number, bbn_handle, &state)) {
                    debug_printf("[PCI]: Couldn't evaluate Root Bus _BBN, skipping this one\n");
                    continue;
                }
                lai_obj_get_integer(&bus_number, &bbn_result);
            } else {
                debug_printf("[PCI]: Root bus doesn't have _BBN, assuming 0\n");
            }

            lai_variable_t seg_number = {};
            uint64_t seg_result = 0;
            lai_nsnode_t *seg_handle = lai_resolve_path(node, "_SEG");
            if (seg_handle) {
                if (lai_eval(&seg_number, seg_handle, &state)){
                    debug_printf("[PCI]: Couldn't evaluate Root Bus _SEG, assuming 0\n");
                } else {
                    lai_obj_get_integer(&seg_number, &seg_result);
                }
            } else {
                debug_printf("[PCI]: Root bus doesn't have _SEG, assuming 0\n");
            }
            enumerate_bus(seg_result, bbn_result);
        }
    }

    for(const auto& entry : pci_devices) debug_printf("[PCI]: Device on %x:%x:%x:%x, class: %s\n", entry.seg, entry.bus, entry.device, entry.function, class_to_str(entry.class_code));
}

uint32_t x86_64::pci::read(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size){
    return internal_read(0, bus, slot, function, offset, access_size);
}

uint32_t x86_64::pci::read(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint8_t access_size){
    return internal_read(seg, bus, slot, function, offset, access_size);
}


void x86_64::pci::write(uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size){
    internal_write(0, bus, slot, function, offset, value, access_size);
}

void x86_64::pci::write(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint16_t offset, uint32_t value, uint8_t access_size){
    internal_write(seg, bus, slot, function, offset, value, access_size);
}

x86_64::pci::bar x86_64::pci::read_bar(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function, uint8_t number){
    x86_64::pci::bar ret = {};
    if(number > 5){
        // Invalid BAR
        ret.type = x86_64::pci::bar_type_invalid;
        return ret;
    }
    ret.number = number;

    uint32_t offset = (0x10 + (number * 4));

    uint32_t bar = x86_64::pci::read(seg, bus, slot, function, offset, 4);

    if(bitops<uint32_t>::bit_test(bar, 0)){
        // IO space
        ret.type = x86_64::pci::bar_type_io;
        ret.base = (bar & 0xFFFFFFFC) & 0xFFFF;

        x86_64::pci::write(seg, bus, slot, function, offset, 0xFFFFFFFF, 4);
        ret.len = ~((x86_64::pci::read(seg, bus, slot, function, offset, 4) & ~(0x3))) + 1;
        ret.len &= 0xFFFF;
        x86_64::pci::write(seg, bus, slot, function, offset, bar, 4);
    } else {
        // MMIO space
        ret.type = x86_64::pci::bar_type_mem;

        uint8_t bar_type = (bar >> 1) & 0x3;
        switch (bar_type)
        {
        case 0x0: // 32bits
            ret.base = (bar & 0xFFFFFFF0);
            break;

        case 0x2: // 64bits
            ret.base = ((bar & 0xFFFFFFF0) | ((uint64_t)x86_64::pci::read(seg, bus, slot, function, offset + 4, 4) << 32));
            break;
        
        default:
            debug_printf("[PCI]: Unknown mmio bar type: %x\n", bar_type);
            break;
        }

        if(bitops<uint32_t>::bit_test(bar, 3)) bitops<uint32_t>::bit_set(bar, x86_64::pci::bar_flags_prefetchable);

        // This is to read the len, it's a tad weird I know
        x86_64::pci::write(seg, bus, slot, function, offset, 0xFFFFFFFF, 4);
        ret.len = ~((x86_64::pci::read(seg, bus, slot, function, offset, 4) & ~(0xF))) + 1;
        x86_64::pci::write(seg, bus, slot, function, offset, bar, 4);
    }

    return ret;
}

x86_64::pci::bar x86_64::pci::read_bar(uint8_t bus, uint8_t slot, uint8_t function, uint8_t number){
    return x86_64::pci::read_bar(0, bus, slot, function, number);
}

x86_64::pci::device x86_64::pci::iterate(x86_64::pci::pci_iterator& iterator){
    uint64_t i = 0;
    for(const auto& entry : pci_devices){
        if(i == iterator){
            ++iterator;
            return entry;
        } 
        i++;
    }

    return x86_64::pci::device();
}