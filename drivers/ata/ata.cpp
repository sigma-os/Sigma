#include "ata.hpp"
#include <iostream>
#include <cstring>

namespace {
    uint8_t inb(uint16_t port){
        uint8_t ret;
        asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }

    void outb(uint16_t port, uint8_t value){
        asm volatile ("out %0, %1" : : "a"(value), "Nd"(port));
    }

    uint16_t inw(uint16_t port){
        uint16_t ret;
        asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }

    uint32_t ind(uint16_t port){
        uint32_t ret;
        asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
        return ret;
    }
}

uint8_t ata::controller::read_cmd(uint8_t reg){
    return inb(this->base.first + reg);
}

uint8_t ata::controller::read_control(uint8_t reg){
    return inb(this->base.second + reg);
}

void ata::controller::write_cmd(uint8_t reg, uint8_t val){
    outb(this->base.first + reg, val);
}

void ata::controller::write_control(uint8_t reg, uint8_t val){
    outb(this->base.second + reg, val);
}

void ata::controller::ns_wait(){
    read_control(control_registers::alt_status);
    read_control(control_registers::alt_status);
    read_control(control_registers::alt_status);
    read_control(control_registers::alt_status);
}

bool ata::controller::wait_busy_clear(){
    while(true){
        auto status = regs::status{.raw = read_control(control_registers::alt_status)};
        if(!status.busy)
            return true;

        if(status.error || status.drive_fault)
            return false;
    }
}

bool ata::controller::wait_for_clear(uint8_t bit){
    std::cerr << "Waiting, " << uint64_t{bit} << "\n";
    while(true){
        auto status = regs::status{.raw = read_control(control_registers::alt_status)};
        if(status.busy)
            continue;
        
        if(!(status.raw & (1 << bit)))
            return true;

        if(status.error || status.drive_fault)
            return false;
        
    }
    std::cerr << "done\n";
}

bool ata::controller::wait_for_set(uint8_t bit){
    std::cerr << "Waiting, " << uint64_t{bit} << "\n";
    while(true){
        auto status = regs::status{.raw = read_control(control_registers::alt_status)};
        std::cout << "Status: 0x" << std::hex << uint64_t{status.raw} << std::endl;
        if(status.busy)
            continue;
        
        if(status.raw & (1 << bit))
            return true;

        if(status.error || status.drive_fault)
            return false;
        
    }
    std::cerr << "done\n";
}

void ata::controller::select_drive(bool slave, bool lba, uint8_t lba28_nibble){
    namespace cmd = command_registers;
    namespace control = control_registers;

    static regs::drive_select cur_selected = {};

    regs::drive_select val = {};
    val.chs_lba = lba;
    val.channel_select = slave;
    val.lba28_high_nibble = lba28_nibble;
    val.obsolete1 = 1;
    val.obsolete = 1;

    if(val.raw == cur_selected.raw)
        return;

    regs::status status{.raw = read_cmd(cmd::status)};
    if(status.busy || status.drq){
        std::cout << "ata: Can't select drive" << std::endl;
        return;
    }

    write_cmd(cmd::drive_select, val.raw);
    ns_wait();
    read_cmd(cmd::status);

    status = {.raw = read_cmd(cmd::status)};
    if(status.busy || status.drq){
        std::cout << "ata: Wasn't able to select drive" << std::endl;
        return;
    }

    cur_selected = val;

    return;
}

void ata::controller::identify_16(uint8_t* data, disk& device){
    namespace cmd = command_registers;
    namespace control = control_registers;

    auto* _data = (uint16_t*)data;

    this->select_drive(device.slave, false, 0);

    wait_busy_clear();
    write_cmd(cmd::features, 0);
    write_cmd(cmd::sector_count, 0);
    write_cmd(cmd::lba_low, 0);
    write_cmd(cmd::lba_mid, 0);
    write_cmd(cmd::lba_high, 0);
    write_cmd(cmd::command, commands::identify);
    ns_wait();
    //if(wait_for_set(regs::status::raw_bits::drq))
    //    for(size_t i = 0; i < 256; i++)
    //        _data[i] = inw(cmd::data);

    while(!(read_control(control_registers::alt_status) & 0x08));
    for(size_t i = 0; i < 256; i++)
        _data[i] = inw(cmd::data);
}


void ata::controller::identify_32(uint8_t* data, disk& device){
    namespace cmd = command_registers;
    namespace control = control_registers;

    auto* _data = (uint32_t*)data;

    this->select_drive(device.slave, false, 0);

    wait_busy_clear();
    write_cmd(cmd::features, 0);
    write_cmd(cmd::sector_count, 0);
    write_cmd(cmd::lba_low, 0);
    write_cmd(cmd::lba_mid, 0);
    write_cmd(cmd::lba_high, 0);
    write_cmd(cmd::command, commands::identify);
    ns_wait();
    //if(wait_for_set(regs::status::raw_bits::drq))
    //    for(size_t i = 0; i < 128; i++)
    //        _data[i] = ind(cmd::data);

    while(!(read_control(control_registers::alt_status) & 0x08));
    for(size_t i = 0; i < 128; i++)
        _data[i] = ind(cmd::data); 
}

bool ata::controller::check_identity(uint8_t* data){
    if(data[510] == 0xA5){
        uint8_t check = 0;
        for(size_t i = 0; i < 511; i++)
            check += data[i];

        return ((uint8_t)(-check) == data[511]);
    } else if(data[510] != 0)
        return false;
    else
        return true;  
}


void ata::controller::init_drive(disk& device){
    namespace cmd = command_registers;
    namespace control = control_registers;

    write_cmd(cmd::drive_select, device.slave ? (1 << 4) : 0);
    ns_wait();
    write_control(control::device_control, 1 << 2);
    for(int i = 0; i < 10000; i++)
        asm("pause");
    write_control(control::device_control, 0);
    for(int i = 0; i < 20000; i++)
        asm("pause");
    
    std::cerr << "CCCCCCCCCC\n";

    wait_busy_clear();

    std::cerr << "AAAAAAAAAa\n";

    for(int i = 0; i < 50000; i++)
        asm("pause");

    if(read_cmd(cmd::error) & (1 << 7)){
        std::cout << "ata: reset failed" << std::endl;
        return;
    }

    std::cerr << "BBBBBBBBB\n";

    device.id = {read_cmd(cmd::lba_high), read_cmd(cmd::lba_mid)};
    if(device.id != std::pair{uint8_t{0x00}, uint8_t{0x00}}){
        std::cout << "ata: Unknown device id: 0x" << std::hex << uint64_t{device.id.first} << " : 0x" << uint64_t{device.id.second} << std::endl;
        return;
    }

    std::cout << "ata: Detected device with ID Pair 0x" << std::hex << uint64_t{device.id.first} << " : 0x" << uint64_t{device.id.second} << std::endl;

    std::memset((void*)(device.identity), 0xFF, 512);
    this->identify_16(device.identity, device);
    if(!this->check_identity(device.identity)){
        std::cout << "ata: Failed identity checksum" << std::endl;
        return;
    }

    std::cout << "haha yes" << std::endl;

    uint8_t identity_32[512] = {};
    this->identify_32(identity_32, device);
    
    if(std::memcmp((void*)device.identity, (void*)identity_32, 512) == 0)
        device.flags.pio32 = 1;

    //if(((uint16_t*)device.identity)[49] & (1 << 9)){
        device.flags.lba = 1;
    //}
    device.flags.lba48 = (device.identity[167] & (1 << 2)) && (device.identity[173] & (1 << 2));

    if(!device.flags.lba){
        std::cout << "ata: No LBA support detected, not furthering initialization" << std::endl;
        return;
    }

    char model[41] = {};
	std::memcpy(model, device.identity + 54, 40);
	model[40] = '\0';

	// model name is returned as big endian, swap each 2-byte pair to fix that
	for (size_t i = 0; i < 40; i += 2) {
		auto tmp = model[i];
		model[i] = model[i + 1];
		model[i + 1] = tmp;
    }

    std::cout << "ata: Identified device, model: " << model << ", pio32: " << (device.flags.pio32 ? ("supported") : ("unsupported")) << ", lba48: " << (device.flags.lba48 ? ("supported") : ("unsupported")) << std::endl;

    device.flags.exists = 1;
}

void ata::controller::init(){
    namespace cmd = command_registers;
    namespace control = control_registers;

    auto status = read_cmd(cmd::status);
    if(status == 0xFF){
        std::cout << "ata: No devices on channel" << std::endl;
        return;
    }

    write_cmd(cmd::sector_count, 0x2A);
    write_cmd(cmd::sector_number, 0x55);

    for(int i = 0; i < 150000; i++)
        asm("pause");

    if(read_cmd(cmd::sector_count) != 0x2A || read_cmd(cmd::sector_number) != 0x55){
        std::cout << "ata: ports not retaining values" << std::endl;
        return;
    }

    this->ports[0] = {};
    this->ports[0].slave = false;
    this->init_drive(this->ports[0]);

    this->ports[1] = {};
    this->ports[1].slave = true;
    this->init_drive(this->ports[1]);
}