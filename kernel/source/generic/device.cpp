#include <Sigma/generic/device.h>
#include <Sigma/proc/process.h>
#include <Sigma/generic/user_handle.hpp>

#include <klibcxx/mutex.hpp>

misc::lazy_initializer<types::vector<generic::device::device>> device_list;

types::vector<generic::device::device>& generic::device::get_device_list(){
    return *device_list;
}

void generic::device::init(){
    device_list.init();
}

void generic::device::print_list(){
    debug_printf("[DEVICE]: Printing device list\n");
    for(auto& entry : *device_list){
        debug_printf("    %s\n", entry.name ? entry.name : "Unknown");
    }
}

void generic::device::add_pci_device(x86_64::pci::device* dev){
    auto& entry = *generic::device::get_device_list().empty_entry();
    entry.add_pci_device(dev);
    entry.name = x86_64::pci::class_to_str(dev->class_code, dev->subclass_code, dev->prog_if);
}

/*static device::device_descriptor find_acpi_node(lai_nsnode_t* node){
    for(auto& entry : *device_list){
        if(entry.contact.acpi && entry.acpi_contact.node == node)
            return &entry - device_list->begin();
    }
    return UINT64_MAX;
}*/

static generic::device::device_descriptor find_pci_node(uint16_t seg, uint8_t bus, uint8_t slot, uint8_t function){
    for(auto& entry : *device_list){
        if(entry.contact.pci && entry.pci_contact.device->seg == seg && entry.pci_contact.device->bus == bus && entry.pci_contact.device->device == slot && entry.pci_contact.device->function == function)
            return &entry - device_list->begin();
    }
    return UINT64_MAX;
}

static generic::device::device_descriptor find_pci_class_node(uint8_t class_code, uint8_t subclass_code, uint8_t prog_if, uint64_t index){
    uint64_t i = 0;
    for(auto& entry : *device_list){
        if(entry.contact.pci && entry.pci_contact.device->class_code == class_code && entry.pci_contact.device->subclass_code == subclass_code && entry.pci_contact.device->prog_if == prog_if){
            if(i == index)
                return &entry - device_list->begin();
            else
                i++;
        }
    }
    
    return UINT64_MAX;
}

static bool get_resource_region(generic::device::device_descriptor dev, uint64_t origin, uint8_t index, generic::device::device::resource_region* data){
    if(dev >= device_list->size())
        return false;
    auto& device = device_list->operator[](dev);

    switch (origin)
    {
    case generic::device::device::resource_region::originPciBar: {
        ASSERT(index <= 6);
        ASSERT(device.contact.pci);
        auto& bar = device.pci_contact.device->bars[index];
        *data = {.type = bar.type, .origin = generic::device::device::resource_region::originPciBar, .base = bar.base, .len = bar.len};
        break;

    }        
    default:
        debug_printf("[DEVICE]: Unknown devctl get_resource_region origin: %x\n", origin);
        break;
    }
    return true;
}

static std::mutex devctl_lock{};

uint64_t generic::device::devctl(uint64_t cmd, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, x86_64::idt::idt_registers* regs){
    std::lock_guard lock{devctl_lock};
    uint64_t ret = UINT64_MAX;
    switch (cmd)
    {
    case generic::device::devctl_cmd_nop:
        #ifdef LOG_SYSCALLS
        debug_printf("[DEVICE]: Handled cmd_nop\n");
        #endif
        break;

    case generic::device::devctl_cmd_claim: {
        #ifdef LOG_SYSCALLS
        debug_printf("[DEVICE]: Handling cmd_claim, descriptor: %d\n", arg1);
        #endif
        auto& device = device_list->operator[](arg1);
        if(device.driver == 0) {
            device.driver = proc::process::get_current_tid();
            ret = 0;
        } else {
            ret = 1;
        }
        break;
    }

    case generic::device::devctl_cmd_find_pci:
        #ifdef LOG_SYSCALLS
        debug_printf("[DEVICE]: Handling cmd_find_pci, %x:%x:%x:%x\n", arg1, arg2, arg3, arg4);
        #endif
        ret = find_pci_node(arg1, arg2, arg3, arg4);
        break;

    case generic::device::devctl_cmd_find_pci_class:
        #ifdef LOG_SYSCALLS
        debug_printf("[DEVICE]: Handling cmd_find_pci_class, %d:%d:%d, index: %d\n", arg1, arg2, arg3, arg4);
        #endif
        ret = find_pci_class_node(arg1, arg2, arg3, arg4);
        break;
    
    case generic::device::devctl_cmd_get_resource_region:
        #ifdef LOG_SYSCALLS
        debug_printf("[DEVICE]: Handling cmd_get_resouse_region, descriptor: %d, origin: %d, index: %d, ptr: %x\n", arg1, arg2, arg3, arg4);
        #endif
        ret = get_resource_region(arg1, arg2, arg3, reinterpret_cast<device::resource_region*>(arg4));
        break;

    case generic::device::devctl_cmd_enable_irq: {
        auto& device = device_list->operator[](arg1);

        ASSERT(device.contact.pci);

        auto vec = x86_64::idt::get_free_vector();
        device.pci_contact.device->install_msi(0, vec);

        auto* irq = new handles::irq_handle{vec};

        x86_64::idt::register_interrupt_handler({.vector = vec, .callback = +[](MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* r, void* userptr){
            auto* event = (generic::event*)userptr;

            event->trigger();
        }, .userptr = (void*)&irq->event, .is_irq = true});

        ret = proc::process::get_current_thread()->handle_catalogue.push(irq);
        break;
    }

    case generic::device::devctl_cmd_wait_on_irq: {
        auto* thread = proc::process::get_current_thread();
        auto& irq = *thread->handle_catalogue.get<generic::handles::irq_handle>(arg1);

        thread->block(&irq.event, regs);
        break;
    }

    case generic::device::devctl_cmd_read_pci: {
        auto& device = device_list->operator[](arg1);
        auto& pci_dev = *device.pci_contact.device;
        ret = x86_64::pci::read(pci_dev.seg, pci_dev.bus, pci_dev.device, pci_dev.function, arg2, arg3);
        break;
    }

    case generic::device::devctl_cmd_write_pci: {
        auto& device = device_list->operator[](arg1);
        auto& pci_dev = *device.pci_contact.device;
        x86_64::pci::write(pci_dev.seg, pci_dev.bus, pci_dev.device, pci_dev.function, arg2, arg4, arg3);
        break;
    }

    default:
        debug_printf("[DEVICE]: Unknown command: %x\n", cmd);
        break;
    }

    return ret;
}
