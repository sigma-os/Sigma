#include "xhci.hpp"
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <string_view>
#include <iostream>

xhci::controller::controller(uint64_t device_descriptor): device_descriptor{device_descriptor} {
    libsigma_resource_region_t region = {};
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 0, (uint64_t)&region);

    this->capabilities = (volatile cap_regs*)libsigma_vm_map(region.len, nullptr, (void*)region.base, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON);
    this->operation = (volatile operational_regs*)((uintptr_t)this->capabilities + this->capabilities->cap_length);
    this->runtime = (volatile runtime_regs*)((uintptr_t)this->capabilities + this->capabilities->runtime_offset);
    this->doorbells = (volatile uint32_t*)((uintptr_t)this->capabilities + this->capabilities->doorbell_offset);

    printf("xhci: Initializing controller, phys: [0x%lx -> 0x%lx], virt: %p\n", region.base, region.base + region.len, this->capabilities);

    cap_regs::hci_version_t version{.raw = this->capabilities->hci_version};
    printf("      Version: %d.%d.%d\n", version.major, version.minor, version.subminor);

    cap_regs::hcs_params_1_t hcs_1{.raw = this->capabilities->hcs_params[0]};
    printf("      Number of slots: %d\n", hcs_1.max_slots);
    printf("      Number of ports: %d\n", hcs_1.max_ports);
    printf("      Number of IRQs: %d\n", hcs_1.n_irqs);

    this->n_ports = hcs_1.max_ports;

    cap_regs::hcs_params_2_t hcs_2{.raw = this->capabilities->hcs_params[1]};
    printf("      Event Ring Segment Table Max: %d\n", 1 << hcs_2.erst_max);

    this->n_scratchpads = (((hcs_2.raw >> 16) & 0x3e0) | ((hcs_2.raw >> 27) & 0x1f)); // Cursed linux bit magic that doesn't match spec
    printf("      Max Scratchpad buffer: %ld\n", this->n_scratchpads);
    
    cap_regs::hcs_params_3_t hcs_3{.raw = this->capabilities->hcs_params[2]};
    printf("      U1 device exit latency is less than %d microseconds\n", hcs_3.u1_device_exit_latency);
    printf("      U2 device exit latency is less than %d microseconds\n", hcs_3.u2_device_exit_latency);

    cap_regs::hcc_params_1_t hcc_1{.raw = this->capabilities->hcc_params_1};

    this->context_size = (hcc_1.context_size) ? 64 : 32;

    // Bit position in the size register indicates the size, not the value
    uint8_t bit_set = 0;
    for(int i = 0; i < 16; i++){
        if(this->operation->page_size & (1 << i)){
            bit_set = i;
            break;
        }
    }

    this->page_size = 1 << (bit_set + 12);
    printf("      Page size: %x KiB\n", this->page_size / (0x1000 / 4));

    this->irq_descriptor = devctl(devCtlEnableIrq, this->device_descriptor, 0, 0, 0);

    volatile uint32_t* extended_offset = (uint32_t*)((uintptr_t)this->capabilities + (hcc_1.extended_cap_ptr * 4));
    while(true){
        uint8_t type = (*extended_offset & 0xFF);
        uint8_t next_ptr = ((*extended_offset >> 8) & 0xFF);

        if(*extended_offset == ~0)
            break;

        // BIOS -> OS Handoff
        if(type == 1){
            this->bios_os_handoff = (volatile extended_caps::bios_os_handoff*)extended_offset;
            this->do_bios_os_handoff();
        } else if(type == 2) {
            volatile auto* protocol = (volatile extended_caps::supported_protocol_cap*)extended_offset;
            

            extended_caps::supported_protocol_cap::version_t version{.raw = protocol->version};
            extended_caps::supported_protocol_cap::name_t name{.raw = protocol->name};
            extended_caps::supported_protocol_cap::ports_info_t ports_info{.raw = protocol->ports_info};
            extended_caps::supported_protocol_cap::slot_type_t slot_type{.raw = protocol->slot_type};

            supported_protocol proto{.major = version.major_rev, .minor = version.major_rev, .port_offset = ports_info.compatible_port_offset - 1, .port_count = ports_info.compatible_port_count, .slot_type = slot_type.protocol_slot_type};

            this->supported_protocols.push_back(proto);

            printf("xhci: Detected support for protocol %.4s %d.%d\n", name.name_string, version.major_rev, version.minor_rev);
        } else {
            printf("xhci: Unknown extended capability id: %d\n", type);
        }

        if(next_ptr == 0) // End of chain
            break;

        extended_offset += next_ptr;
    }

    uint16_t vendor_id = devctl(devCtlReadPci, this->device_descriptor, 0, 2, 0);
    if(vendor_id == 0x8086)
        this->quirks |= quirkIntel;

    if(quirks & quirkIntel)
        this->intel_enable_xhci_ports();

    this->stop_execution();
    this->reset_controller();

    operational_regs::usbsts sts{.raw = this->operation->status};
    while(true){
        sts.raw = this->operation->status;
        if(sts.controller_not_ready == 0){
            break;
        }
    }

    operational_regs::config_reg config{};
    config.slot_enable = hcs_1.max_slots; // Enable all slots
    this->operation->config = config.raw;

    this->dcbaap_size = (hcs_1.max_slots + 1) + 1; // 1 for every slot + scratchbuffer array
    assert(this->dcbaap_size <= 256);

    libsigma_phys_region_t dcbaab_region = {};
    if(libsigma_get_phys_region(this->dcbaap_size * sizeof(uint64_t), PROT_READ | PROT_WRITE, MAP_ANON, &dcbaab_region)){
        printf("xhci: Failed to allocate physical region for dcbaap\n");
        return;
    }

    this->dcbaap = (uint64_t*)dcbaab_region.virtual_addr;
    this->operation->device_context_base = dcbaab_region.physical_addr;

    if(this->n_scratchpads){
        libsigma_phys_region_t scratch_array_region = {};
        if(libsigma_get_phys_region(this->n_scratchpads * sizeof(uint64_t), PROT_READ | PROT_WRITE, MAP_ANON, &scratch_array_region)){
            printf("xhci: Failed to allocate physical region for scratchpad buffer array\n");
            return;
        }

        this->dcbaap[0] = scratch_array_region.physical_addr;

        auto* scratchpad_buffer_array = (uint64_t*)scratch_array_region.virtual_addr;

        for(size_t i = 0; i < this->n_scratchpads; i++){
            libsigma_phys_region_t pad = {};
            if(libsigma_get_phys_region(this->page_size, PROT_READ | PROT_WRITE, MAP_ANON, &pad)){
                printf("xhci: Failed to allocate physical region for scratchpad buffer array\n");
                return;
            }

            scratchpad_buffer_array[i] = pad.physical_addr;
        }
    }

    this->operation->command_ring_control = this->cmd_ring.get_phys_base() | 1;

    libsigma_phys_region_t erst_region = {};
    if(libsigma_get_phys_region(4096, PROT_READ | PROT_WRITE, MAP_ANON, &erst_region)){
        printf("xhci: Failed to allocate physical region for erst\n");
        return;
    }

    this->erst = (event_ring_segment_table_entry*)erst_region.virtual_addr;
    this->erst[0].base = this->evt_ring.get_phys_base();
    this->erst[0].size = this->evt_ring.get_n_entries();

    this->runtime->interrupter[0].erst_dequeue = this->evt_ring.get_phys_base();//erdp.raw;
    this->runtime->interrupter[0].erst_size = 1; // Only 1 event ring for now
    this->runtime->interrupter[0].erst_base = erst_region.physical_addr;

    runtime_regs::interrupter_t::iman_t iman{.raw = this->runtime->interrupter[0].iman};
    iman.irq_pending = 0;
    iman.irq_enable = 1;
    this->runtime->interrupter[0].iman = iman.raw;


    this->start_execution();

    operational_regs::usbcmd cmd{.raw = this->operation->command};
    cmd.irq_enable = 1;
    this->operation->command = cmd.raw;

    printf("xhci: Enabled IRQs and started execution engine\n");

    this->ports.resize(this->n_ports);

    this->init_ports();
}

void xhci::controller::do_bios_os_handoff(){
    extended_caps::bios_os_handoff::capability_reg cap{.raw = this->bios_os_handoff->capability};
    if(cap.bios_owned_semaphore){
        printf("xhci: Device is bios owned, requesting control...");

        cap.os_owned_semaphore = 1;
        this->bios_os_handoff->capability = cap.raw;
        while(true){
            cap.raw = this->bios_os_handoff->capability;
            if(cap.bios_owned_semaphore == 0){
                printf("done\n");
                return;
            }
        }
    }
}

void xhci::controller::intel_enable_xhci_ports(){
    // Certain Sony VAIO laptops don't support switching ports to xHCI
    if(devctl(devCtlReadPci, this->device_descriptor, 0x2C, 2, 0) == 0x104D && devctl(devCtlReadPci, this->device_descriptor, 0x2E, 2, 0) == 0x90A8)
        return;

    uint32_t usb3_enable = devctl(devCtlReadPci, this->device_descriptor, 0xDC, 4, 0);
    devctl(devCtlWritePci, this->device_descriptor, 0xD8, 4, usb3_enable);

    uint32_t usb2_enable = devctl(devCtlReadPci, this->device_descriptor, 0xD4, 4, 0);
    devctl(devCtlWritePci, this->device_descriptor, 0xD0, 4, usb2_enable);
    
    uint32_t switched = devctl(devCtlReadPci, this->device_descriptor, 0xD0, 4, 0);
    printf("xhci: Switched %d/%d ports to xHCI\n", usb3_enable, switched);
}

void xhci::controller::reset_controller(){
    operational_regs::usbsts sts{.raw = this->operation->status};
    operational_regs::usbcmd cmd{.raw = this->operation->command};
    assert(sts.halted);
    cmd.reset = 1;
    this->operation->command = cmd.raw;

    printf("xhci: Resetting controller...");

    if(this->quirks & quirkIntel)
        printf("xhci: TODO: Wait 1000ms as for quirk\n");

    while(true){
        cmd.raw = this->operation->command;
        if(cmd.reset == 0){
            printf("done\n");
            return;
        }
    }
}

void xhci::controller::stop_execution(){
    operational_regs::usbcmd cmd{.raw = this->operation->command};
    cmd.run = 0;
    this->operation->command = cmd.raw;

    printf("xhci: Stopping controller execution...");

    operational_regs::usbsts sts{.raw = this->operation->status};
    while(true){
        sts.raw = this->operation->status;
        if(sts.halted == 1){
            printf("done\n");
            return;
        }
    }
}

void xhci::controller::start_execution(){
    operational_regs::usbcmd cmd{.raw = this->operation->command};
    cmd.run = 1;
    this->operation->command = cmd.raw;

    printf("xhci: Starting controller execution...");

    operational_regs::usbsts sts{.raw = this->operation->status};
    while(true){
        sts.raw = this->operation->status;
        if(sts.halted == 0){
            printf("done\n");
            return;
        }
    }
}

xhci::command_completion_event_trb xhci::controller::send_command(xhci::raw_trb* trb){
    auto index = this->cmd_ring.enqueue_trb(trb, false);

    this->doorbells[0] = 0; // Ring xHCI command ring doorbell
    
    while(!this->cmd_ring.events[index].finished)
        this->handle_irq();

    return this->cmd_ring.events[index].trb;
}

void xhci::controller::handle_irq(){
    devctl(devCtlWaitOnIrq, this->irq_descriptor, 0, 0, 0);

    // Acknowledge IRQ
    //volatile uint32_t tmp = this->operation->status;
    //this->operation->status = tmp;

    auto* event = (raw_trb*)this->evt_ring.dequeue();
    uint64_t dequeue = 0;
    while(event->cycle == this->evt_ring.cycle()){
        switch (event->trb_type)
        {
        case trb_types::transferEvent: {
            auto* completion = (transfer_completion_event_trb*)event;
            transfer_ring* ring;
            if(completion->endpoint_id == 1){
                ring = &this->port_by_slot_id[completion->slot_id]->command;
            } else {
                printf("xhci: Unknown EP ID\n");
                break;
            }

            uint64_t index = (completion->trb_ptr - ring->get_phys_base()) / 0x10;

            if(index == 0xfe)
                index = -1;

            ring->events[index + 1].finished = true;
            ring->events[index + 1].trb = *completion;
            break;
        }
        case trb_types::commandCompletionEvent: {
            auto* completion = (command_completion_event_trb*)event;
            int index = (completion->trb_ptr - this->cmd_ring.get_phys_base()) / 0x10;

            this->cmd_ring.events[index].finished = true;
            this->cmd_ring.events[index].trb = *completion;
            break;
        }
        case trb_types::portStatusChangeEvent: {
            printf("xhci: Port status change event\n");
            break;
        }
        
        default:
            printf("xhci: Unknown event trb: %d\n", event->trb_type);
            break;
        }

        this->evt_ring.dequeue() = (uint64_t)(event + 1);

        uint64_t index = this->evt_ring.dequeue() - (uintptr_t)this->evt_ring.get_entries();
        uint64_t val = (uint64_t)this->evt_ring.get_phys_base();
        val += (index % 0x1000);
        dequeue = val;
        if(!(index % 4096)){
            this->evt_ring.dequeue() = (uint64_t)this->evt_ring.get_entries();
            this->evt_ring.cycle() = !this->evt_ring.cycle();
        }

        event = (raw_trb*)this->evt_ring.dequeue();
    }

    this->runtime->interrupter[0].iman |= 1; // Clear IRQ Pending

    this->runtime->interrupter[0].erst_dequeue = dequeue | (1 << 3);
}

#pragma region Port functions

void xhci::controller::init_ports(){
    this->setup_port_protocols();

    // First try to reset all USB 3 ports
    for(size_t i = 0; i < this->n_ports; i++)
        if(ports[i].active && ports[i].protocol->major == 3)
            ports[i].active = this->reset_port(i);
    
    // In some cases the USB 3 reset might fail, then we can still try it in USB2 mode
    // Also just enable all USB 2 ports
    for(size_t i = 0; i < this->n_ports; i++)
        if(ports[i].active && ports[i].protocol->major == 2)
            ports[i].active = this->reset_port(i);

    for(size_t i = 0; i < this->n_ports; i++){
        auto& port = this->ports[i];
        if(!port.active)
            continue;

        volatile operational_regs::port_regs* regs = &this->operation->ports[i];

        operational_regs::port_regs::control_reg control{.raw = regs->control};
        port.speed = control.port_speed;

        switch(port.speed)
        {
        case speedFull:
            printf("xhci: TODO: Full speed packet size\n");
            break;
        case speedLow:
            printf("xhci: Port is LowSpeed\n");
            port.packet_size = 8;
            break;
        case speedHi:
            printf("xhci: Port is HighSpeed\n");
            port.packet_size = 64;
            break;
        case speedSuper:
            printf("xhci: Port is SuperSpeed\n");
            port.packet_size = 512;
            break;
        default:
            printf("xhci: Unknown port speed %d\n", port.speed);
            break;
        }


        enable_slot_command_trb enable_slot{};
        enable_slot.slot_type = port.protocol->slot_type;
        auto completion = this->send_command((raw_trb*)&enable_slot);

        if(completion.code != trb_completion_codes::success){
            printf("xhci: Failed EnableSlot command, code: %x\n", completion.code);
            continue;
        }

        port.slot_id = completion.slot_id;
        printf("xhci: SlotID for Port %d is %d\n", i, port.slot_id);

        port.in_ctx = in_context{context_size, 0x1000};
        port.out_ctx = out_context{context_size, 0x1000};
        port.command = transfer_ring{};

        this->dcbaap[port.slot_id] = port.out_ctx.phys;

        volatile auto& control_ctx = *port.in_ctx.get_control_ctx();
        control_ctx.add_flags = 0x3;
        control_ctx.drop_flags = 0;

        volatile auto& slot = *port.in_ctx.get_slot_ctx();
        slot.context_entries = 1;
        slot.speed = port.speed;
        slot.root_hub_port_number = i + 1;

        volatile auto& ep0 = *port.in_ctx.get_ep0_ctx();
        ep0.endpoint_type = ep_types::epTypeControlBi;
        ep0.error_count = 3;
        ep0.max_burst_size = 0;
        ep0.max_packet_size = port.packet_size;

        ep0.average_trb_len = 8; // All Control EPs are 8

        ep0.tr_dequeue_ptr = port.command.get_phys_base() | port.command.cycle();

        address_device_command_trb address_dev{};
        address_dev.slot_id = port.slot_id;
        address_dev.in_ctx_ptr = port.in_ctx.phys;
        address_dev.block_set_address_req = 0;
        completion = this->send_command((raw_trb*)&address_dev);

        if(completion.code != trb_completion_codes::success){
            printf("xhci: Failed AddressDevice command, code: %x\n", completion.code);
            continue;
        }

        control_ctx.add_flags = 1;

        configure_ep_trb configure_ep{};
        configure_ep.slot_id = port.slot_id;
        configure_ep.in_ctx_ptr = port.in_ctx.phys;
        completion = this->send_command((raw_trb*)&configure_ep);

        if(completion.code != trb_completion_codes::success){
            printf("xhci: Failed ConfigureEp command, code: %x\n", completion.code);
            continue;
        }

        port_by_slot_id[port.slot_id] = &port;

        printf("xhci: Configured port: %d\n", i);

        usb::packet p{};
        p.request_type = usb::requestDirectionDeviceToHost;
        p.request = usb::requestGetDescriptor;
        p.value = (1 << 8);
        p.length = sizeof(usb::device_descriptor);

        usb::device_descriptor desc{};

        this->send_control(i, p, (uint8_t*)&desc, 0);

        /*p.value = (usb::descriptorTypeString << 8) | 0;
        p.index = 0x0409; // en-US
        p.length = sizeof(usb::string_langid_descriptor);

        usb::string_langid_descriptor langs_desc{};

        this->send_control(i, p, (uint8_t*)&langs_desc, 0);
        
        printf("LANGIDs: ");
        for(int i = 0; i < (langs_desc.length - 2) / 2; i++)
            printf("0x%04x ", langs_desc.langids[i]);
        printf("\n");*/

        auto print_str = [this, i](const char* str, uint8_t index){
            if(!index)
                return;

            usb::packet p{};
            p.request_type = usb::requestDirectionDeviceToHost;
            p.request = usb::requestGetDescriptor;
            p.value = (usb::descriptorTypeString << 8) + index;
            p.index = 0x0409; // en-US
            p.length = sizeof(usb::string_unicode_descriptor);

            usb::string_unicode_descriptor str_desc{};

            this->send_control(i, p, (uint8_t*)&str_desc, 0);
            printf("%s: ", str);
            for(int i = 0; i < (str_desc.length - 2) / 2; i++)
                printf("%c", str_desc.str[i]);
            printf("\n");
        };

        print_str("      Manufacturer: ", desc.manufacturer);
        print_str("      Product: ", desc.product);
        print_str("      Serial Number: ", desc.serial_number);        

        printf("      VendorID: 0x%04x\n", desc.vendor_id);
        printf("      ProductID: 0x%04x\n", desc.product_id);
        printf("      Class: 0x%02x\n", desc.device_class);
        printf("      Subclass: 0x%02x\n", desc.device_sub_class);
    }
}

void xhci::controller::setup_port_protocols(){
    uint32_t usb2_i = 0, usb3_i = 0;

    for(auto& protocol : this->supported_protocols){
        if(protocol.major == 2){
            for(size_t i = 0; i < protocol.port_count; i++){
                auto& port = this->ports[protocol.port_offset + i];
                port.protocol = &protocol;
                port.offset = usb2_i++;
            }
        }
    }

    for(auto& protocol : this->supported_protocols){
        if(protocol.major == 3){
            for(size_t i = 0; i < protocol.port_count; i++){
                auto& port = this->ports[protocol.port_offset + i];
                port.protocol = &protocol;
                port.offset = usb3_i++;
            }
        }
    }

    // Pair up USB2 and 3 ports
    for(size_t i = 0; i < this->n_ports; i++){
        for(size_t j = 0; j < this->n_ports; j++){
            auto& a = this->ports[i];
            auto& b = this->ports[j];
            if(a.offset == b.offset && a.protocol != b.protocol){
                a.has_pair = true;
                b.has_pair = true;

                a.other_port_num = j;
                b.other_port_num = i;
            }
        }
    }

    // Mark all ports active, but deactivate USB2 ports that have a USB3 pair
    for(size_t i = 0; i < this->n_ports; i++)
        if(this->ports[i].protocol->major == 3 || (this->ports[i].protocol->major == 2 && !this->ports[i].has_pair))
            this->ports[i].active = true;
}

bool xhci::controller::reset_port(size_t port_num){
    auto& p = this->ports[port_num];
    volatile operational_regs::port_regs* regs = &this->operation->ports[port_num];

    // Port is unpowered, try to power up
    if(!(regs->control & (1 << 9))){
        regs->control = 1 << 9;

        // TODO: timeout 20millisecs
        for(volatile uint64_t i = 0; i < 200000; i++)
            asm("pause");

        if(!(regs->control & (1 << 9)))
            return false;
    }

    // Port is now powered up, make sure that status change bits are clear
    regs->control = (1 << 9) | (1 << 17) | (1 << 18) | (1 << 20) | (1 << 21) | (1 << 22);

    if(p.protocol->major == 3){
        regs->control = (1 << 9) | (1 << 31);
    } else if(p.protocol->major == 2){
        regs->control = (1 << 9) | (1 << 4);
    } else {
        printf("xhci: Trying to reset USB port of unknown revision %d.%d\n", p.protocol->major, p.protocol->minor);
        return false;
    }

    int timeout = 500;
    while(timeout){
        if(regs->control & (1 << 21))
            break;
        
        timeout--;
        // TODO: timeout 20millisecs
        for(volatile uint64_t i = 0; i < 20000; i++)
            asm("pause");
    }
    
    bool reset_successful = false;

    if(timeout > 0){
        // TODO: timeout 3millisecs
        for(volatile uint64_t i = 0; i < 30000; i++)
            asm("pause");

        if(regs->control & (1 << 1)){
            // Clear status change bits
            regs->control = (1 << 9) | ((1<<17) | (1<<18) | (1<<20) | (1<<21) | (1<<22));

            reset_successful = true;
            printf("xhci: Reset port: %d\n", port_num);
        }
    }

    if(reset_successful && p.protocol->major == 2){
        p.active = true;
        if(p.has_pair)
            this->ports[p.other_port_num].active = false;
    }

    if(!reset_successful && p.protocol->major == 3){
        p.active = false;
        this->ports[p.other_port_num].active = true;
    }

    return reset_successful;
}

void xhci::controller::send_control(size_t p, usb::packet packet, uint8_t* data, int write){
    auto& port = this->ports[p];
    auto [setup_trb, _] = port.command.ring.get_trb();

    volatile auto& setup = *(setup_stage_trb*)setup_trb;
    setup.trb_type = trb_types::setupStage;
    setup.bmRequestType = packet.request_type;
    setup.bRequest = packet.request;
    setup.wValue = packet.value;
    setup.wIndex = packet.index;
    setup.wLength = packet.length;
    setup.trb_transfer_len = 8;
    setup.transfer_type = 3; // // TODO: wut is this
    
    setup.immediate_data = 1;

    uint32_t state = (setup_trb->data[3] & 0x1) ? 0 : 1;
    setup_trb->data[3] = state  | (setup_trb->data[3] & ~1);

    libsigma_phys_region_t dma_region = {};
    if(packet.length){
        if(libsigma_get_phys_region(packet.length, PROT_READ | PROT_WRITE, MAP_ANON, &dma_region)){
            printf("xhci: Failed to allocate physical region for dma\n");
            return;
        }

        if(write)
            memcpy((void*)dma_region.virtual_addr, data, packet.length);

        auto [data_trb, __] = port.command.ring.get_trb();
        volatile auto& data = *(data_stage_trb*)data_trb;
        data.trb_type = trb_types::dataStage;
        data.data_ptr = dma_region.physical_addr;
        data.transfer_len = packet.length;
        data.direction = write ? 0 : 1;

        state = (data_trb->data[3] & 0x1) ? 0 : 1;
        data_trb->data[3] = state | (data_trb->data[3] & ~1);
    }

    auto [status_trb, ___] = port.command.ring.get_trb();
    volatile auto& status = *(status_stage_trb*)status_trb;
    status.trb_type = trb_types::statusStage;
    status.irq_on_completion = 1;
    status.direction = write ? 0 : 1;

    state = (status_trb->data[3] & 0x1) ? 0 : 1;
    status_trb->data[3] = state | (status_trb->data[3] & ~1);

    uint32_t index = (port.command.enqueue() - (uint64_t)port.command.get_entries()) / 0x10;

    this->doorbells[port.slot_id] = 1;

    while(!port.command.events[index].finished)
        this->handle_irq();

    if(packet.length && !write)
        memcpy(data, (void*)dma_region.virtual_addr, packet.length);
}

#pragma endregion