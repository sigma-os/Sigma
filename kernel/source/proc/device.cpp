#include <Sigma/proc/device.h>
#include <Sigma/proc/process.h>

misc::lazy_initializer<types::vector<proc::device::device>> device_list;

types::vector<proc::device::device>& proc::device::get_device_list(){
    return *device_list;
}

void proc::device::init(){
    device_list.init();
}

void proc::device::print_list(){
    debug_printf("[DEVICE]: Printing device list\n");
    for(auto& entry : *device_list)
        debug_printf("    %s\n", entry.name ? entry.name : "Unknown");
}

proc::device::device_descriptor proc::device::find_acpi_node(lai_nsnode_t* node){
    for(auto& entry : *device_list){
        if(entry.contact.acpi && entry.acpi_contact.node == node)
            return &entry - device_list->begin();
    }
    return UINT64_MAX;
}

proc::device::device_descriptor proc::device::find_pci_node(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function){
    for(auto& entry : *device_list){
        if(entry.contact.pci && entry.pci_contact.device->seg == seg && entry.pci_contact.device->bus == bus && entry.pci_contact.device->device == slot && entry.pci_contact.device->function == function)
            return &entry - device_list->begin();
    }
    return UINT64_MAX;
}

bool proc::device::get_pci_bar(proc::device::device_descriptor dev, uint8_t bar_index, pci_bar* data){
    auto& device = device_list->operator[](dev);
    if(!device.contact.pci)
        return false;

    auto& bar = device.pci_contact.device->bars[bar_index];

    data->type = bar.type;
    data->base = bar.base;
    data->size = bar.len;
    return true;
}

uint64_t proc::device::devctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4){
    uint64_t ret = UINT64_MAX;
    switch (cmd)
    {
    case proc::device::devctl_cmd_nop:
        debug_printf("[DEVICE]: Handled nop\n");
        break;

    case proc::device::devctl_cmd_claim: {
        debug_printf("[DEVICE]: Handling claim\n");
        auto& device = device_list->operator[](arg1);
        if(device.driver == 0) {
            device.driver = proc::process::get_current_tid();
            ret = 0;
        } else {
            ret = 1;
        }
        break;
    }

    case proc::device::devctl_cmd_find_pci:
        debug_printf("[DEVICE]: Handling cmd_find_pci\n");
        ret = find_pci_node(arg1, arg2, arg3, arg4);
        break;
    
    default:
        debug_printf("[DEVICE]: Unknown command: %x\n", cmd);
        break;
    }

    return ret;
}
