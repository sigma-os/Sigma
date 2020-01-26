#include "ahci.hpp"
#include <libdriver/bit.hpp>

#include <libsigma/memory.h>
#include <iostream>
#include <sys/mman.h>
#include <assert.h>
#include <cstring>

using namespace ahci::regs;


ahci::controller::controller(uintptr_t phys_base, size_t size){
    this->base = (volatile regs::hba_t*)libsigma_vm_map(size, nullptr, (void*)phys_base, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON);
    
    printf("ahci: Initializing controller, phys: [0x%lx -> 0x%lx], virt: %p\n", phys_base, phys_base + size, base);

    // For some reason the prerealease version 0.95 is encoded as 0x09050000 instead of normal so have an exlusion, this should be the only 0.xx version we could encounter
    ghcr_t::vs_t vs{base->ghcr.vs};
    if(vs.major == 0) {
        printf("ahci: Detected controller\n      version: %d.%d\n", vs.major, (vs.minor << 4) + vs.subminor);
        this->major_version = vs.major;
        this->minor_version = (vs.minor << 4) + vs.subminor;
        this->subminor_version = 0;
    } else {
        printf("ahci: Detected controller\n      version: %d.%d.%d\n", vs.major, vs.minor, vs.subminor);
        this->major_version = vs.major;
        this->minor_version = vs.minor;
        this->subminor_version = vs.subminor;
    }

    if(!this->bios_gain_ownership())
        return;

    ghcr_t::ghc_t ghc{base->ghcr.ghc};
    ghc.ea = 1; // Enable AHCI access
    base->ghcr.ghc = ghc.raw;

    ghcr_t::cap_t cap{base->ghcr.cap};

    printf("      Link speed: ");
    switch (cap.iss)
    {
    case 0b0001:
        printf("Gen 1 (1,5 GB/s)\n");
        break;
    case 0b0010:
        printf("Gen 2 (3 GB/s)\n");
        break;
    case 0b0011:
        printf("Gen 3 (6 GB/s)\n");
        break;
    default:
    case 0:
        printf("Reserved\n");
        break;
    }
    printf("      Features: ");
    if(cap.sxs) printf("esata ");
    if(cap.ems) printf("enclosure_management ");
    if(cap.cccs) printf("command_completion_coalescing ");
    if(cap.psc) printf("PM_partial ");
    if(cap.ssc) printf("PM_slumber ");
    if(cap.pmd) printf("PIO_multiple_IRQ ");
    if(cap.fbss) printf("FIS_based_switching ");
    if(cap.spm) printf("port_multiplier ");
    if(cap.sam) printf("AHCI_only ");
    if(cap.sclo) printf("command_list_override ");
    if(cap.sal) printf("activity_led ");
    if(cap.salp) printf("PM_aggressive_link ");
    if(cap.sss) printf("staggered_spinup ");
    if(cap.smps) printf("mechanical_presence_switch ");
    if(cap.ssntf) printf("SNotification_register ");
    if(cap.sncq) printf("native_command_qeueing ");
    if(cap.s64a) printf("64bit_addressing ");

    this->addressing_64bit = (cap.s64a != 0);

    if(this->major_version >= 1 && this->minor_version >= 2){
        ghcr_t::cap2_t cap2{base->ghcr.cap2};

        if(cap2.boh) printf("BIOS_OS_handoff ");
        if(cap2.nvmp) printf("NVMHCI_present ");
        if(cap2.apst) printf("PM_automatic_partial_to_slumber ");
        if(cap2.sds) printf("PM_sleep ");
        if(cap2.sadm) printf("PM_aggressive_sleep_management ");
        if(cap2.deso) printf("PM_sleep_only_from_slumber ");
    }

    printf("\n");

    n_allocated_ports = cap.np + 1;
    printf("      Number of allocated ports: %d\n", n_allocated_ports);

    this->ports = new controller::port[n_allocated_ports];

    n_command_slots = cap.ncs + 1;
    printf("      Number of command slots: %d\n", n_command_slots);

    ghcr_t::pi_t pi{base->ghcr.pi};

    for(int i = 0; i < n_allocated_ports; i++){
        if(pi.is_implemented(i)){
            // Initialize port, just print Sig
            ports[i].regs = &base->ports[i];

            prs_t::ssts_t ssts{ports[i].regs->ssts};
            if(ssts.det != 3) { // Not Device Detected, Comms Active
                if(ssts.det == 0) { // No device on port
                    continue;
                } else if(ssts.det == 1) {
                    std::cerr << "ahci: //TODO: Implement COMRESET\n";
                    return;
                } else if(ssts.det == 4) {
                    std::cerr << "ahci: No comms\n";
                    continue;
                } else {
                    std::cerr << "ahci: Unknown ssts.det value, 0x" << std::hex << ssts.det;
                    continue;
                }
            }

            if(ssts.ipm != 1) { // Device not in active state
                std::cerr << "ahci: //TODO: Move device out of sleep, ssts.ipm: 0x" << std::hex << ssts.det;
                continue;
            }

            prs_t::sig_t sig{ports[i].regs->sig};
            ports[i].type = sig.get_type();

            if(ports[i].type == prs_t::sig_t::device_types::Nothing)
                break;

            printf("ahci: Detected drive at port: %d\n", i);
            printf("      Type: ");
            switch (ports[i].type)
            {
            case prs_t::sig_t::device_types::SATA:
                printf("SATA");
                break;
            case prs_t::sig_t::device_types::SATAPI:
                printf("SATAPI");
                break;
            case prs_t::sig_t::device_types::EnclosureManagementBridge:
                printf("Enclosure Management Bridge");
                break;
            case prs_t::sig_t::device_types::PortMultiplier:
                printf("Port Multiplier");
                break;
            default:
                printf("Unknown");
                break;
            }
            printf("\n");

            libsigma_phys_region_t region = {};
            if(libsigma_get_phys_region(sizeof(port::phys_region), PROT_READ | PROT_WRITE, MAP_ANON, &region)){
                std::cerr << "ahci: Failed to allocate physical region for port\n";
                continue;
            }

            printf("      Region: Phys: %lx, Virt: %lx...", region.physical_addr, region.virtual_addr);

            ports[i].wait_idle();

            ports[i].region = (port::phys_region*)region.virtual_addr;

            ports[i].regs->clb = region.physical_addr & 0xFFFFFFFF;
            if(addressing_64bit) ports[i].regs->clbu = (region.physical_addr >> 32) & 0xFFFFFFFF;

            ports[i].regs->fb = (region.physical_addr & 0xFFFFFFFF) + (sizeof(cmd_header) * 32);
            if(addressing_64bit) ports[i].regs->fbu = (region.physical_addr >> 32) & 0xFFFFFFFF;
            printf("installed\n");

            printf("      Command engine:...");

            prs_t::cmd_t cmd{ports[i].regs->cmd};
            cmd.fre = 1;
            cmd.st = 1;
            ports[i].regs->cmd = cmd.raw;

            ports[i].regs->is = 0xFFFFFFFF;

            for(int i = 0; i < 100000; i++)
                asm("pause");

            cmd = prs_t::cmd_t{ports[i].regs->cmd};
            if(!cmd.cr || !cmd.fr){
                printf("failed\n");
                continue;
            }

            printf("started\n");

            this->identify(ports[i].type == prs_t::sig_t::device_types::SATAPI, ports[i]);

            char serial_number[21] = {};
	        std::memcpy(serial_number, (void*)(ports[i].identification + 20), 20);
	        serial_number[20] = '\0';

	        // model name is returned as big endian, swap each 2-byte pair to fix that
	        for (size_t i = 0; i < 20; i += 2) {
		        auto tmp = serial_number[i];
		        serial_number[i] = serial_number[i + 1];
        		serial_number[i + 1] = tmp;
            }

            printf("      Serial number: %s\n", serial_number);

            char fw_revision[21] = {};
	        std::memcpy(fw_revision, (void*)(ports[i].identification + 46), 8);
	        fw_revision[8] = '\0';

	        // model name is returned as big endian, swap each 2-byte pair to fix that
	        for (size_t i = 0; i < 8; i += 2) {
		        auto tmp = fw_revision[i];
		        fw_revision[i] = fw_revision[i + 1];
        		fw_revision[i + 1] = tmp;
            }

            printf("      Firmware revision: %s\n", fw_revision);

            char model[41] = {};
	        std::memcpy(model, (void*)(ports[i].identification + 54), 40);
	        model[40] = '\0';

	        // model name is returned as big endian, swap each 2-byte pair to fix that
	        for (size_t i = 0; i < 40; i += 2) {
		        auto tmp = model[i];
		        model[i] = model[i + 1];
        		model[i + 1] = tmp;
            }

            printf("      Model: %s\n", model);

            ports[i].lba48 = (ports[i].identification[167] & (1 << 2)) && (ports[i].identification[173] & (1 << 2));

            if(ports[i].type == prs_t::sig_t::device_types::SATA){
                ports[i].n_sectors = *(uint64_t*)(&ports[i].identification[200]);
                if(ports[i].n_sectors == 0){
                    ports[i].n_sectors = *(uint32_t*)&ports[i].identification[120];
                }

                ports[i].bytes_per_sector = 512;
            } else if(ports[i].type == prs_t::sig_t::device_types::SATAPI){
                ports[i].removeable = (ports[i].identification[0] >> 7) == 1;


                auto capacity = this->pi_read_capacity(ports[i]);
                ports[i].n_sectors = capacity.first;
                ports[i].bytes_per_sector = capacity.second;
            } else {
                printf("Unknown type of device\n");
            }
            
            if(ports[i].bytes_per_sector)
                printf("      Bytes per sector: %d\n", ports[i].bytes_per_sector);
            else
                printf("      Bytes per sector: Unknown\n");

            if(ports[i].n_sectors)
                printf("      Number of sectors: %ld [%ld MiB]\n", ports[i].n_sectors, ((ports[i].n_sectors * ports[i].bytes_per_sector) / 1024 / 1024));
            else 
                printf("      Number of sectors: Unknown\n");
            
            printf("      Features: ");
            if(ports[i].lba48) printf("LBA48 ");
            if(ports[i].removeable) printf("Removeable ");
            printf("\n");
        }
    }
}

bool ahci::controller::bios_gain_ownership(){
    if(major_version >= 1 && this->minor_version >= 2){
        ghcr_t::cap2_t cap2{base->ghcr.cap2};

        if(cap2.boh){
            ghcr_t::bohc_t bohc{base->ghcr.bohc};
            bohc.oos = 1; // Request Ownership
            base->ghcr.bohc = bohc.raw;

            for(size_t i = 0; i < 100000; i++) // Wait a bit
                asm("pause");

            bohc = ghcr_t::bohc_t{base->ghcr.bohc};
            if(bohc.bb)
                for(size_t i = 0; i < 800000; i++) // Wait a bit
                    asm("pause");

            bohc = ghcr_t::bohc_t{base->ghcr.bohc};
            if(!bohc.oos && (bohc.bos || bohc.bb)){
                std::cerr << "ahci: Failed to acquire ownership of controller\n";
                return false;
            }

            bohc.ooc = 0;
            base->ghcr.bohc = bohc.raw;
            std::cout << "ahci: Gained ownership of controller\n";
        }
    }
    return true;
}

void ahci::controller::port::wait_idle(){
    prs_t::cmd_t cmd{this->regs->cmd};
    cmd.st = 0;
    this->regs->cmd = cmd.raw;

    while(1){
        prs_t::cmd_t poll{this->regs->cmd};
        if(!poll.cr)
            break;
    }

    cmd = prs_t::cmd_t{this->regs->cmd};
    cmd.fre = 0;
    this->regs->cmd = cmd.raw;

    while(1){
        prs_t::cmd_t poll{this->regs->cmd};
        if(!poll.fr)
            break;
    }
}

void ahci::controller::port::wait_ready(){
    while(1){
        prs_t::tfd_t tfd{this->regs->tfd};
        if(!tfd.status.busy && !tfd.status.drq)
            break;

        asm("pause");
    }
}

std::pair<int, ahci::regs::command_table_t*> ahci::controller::allocate_command(ahci::controller::port& port, size_t fis_size){
    auto index = get_free_command_slot(port);
    if(index == -1)
        return {-1, nullptr}; // No free command slot available rn

    assert(index < 32);

    auto& header = port.region->command_headers[index];

    libsigma_phys_region_t region = {};
    if(libsigma_get_phys_region(fis_size, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "ahci: Failed to allocate physical region for h2d FIS\n";
        return {-1, nullptr};
    }

    header.ctba = region.physical_addr & 0xFFFFFFFF;
    if(addressing_64bit) header.ctbau = (region.physical_addr >> 32) & 0xFFFFFFFF;

    return {index, (ahci::regs::command_table_t*)region.virtual_addr};
}

int ahci::controller::get_free_command_slot(ahci::controller::port& port){
    for(int i = 0; i < n_command_slots; i++)
        if((port.regs->ci & (1u << i)) == 0)
            return i;
                    
    return -1;
}

void ahci::controller::identify(bool packet_device, ahci::controller::port& port){
    auto slot = allocate_command(port, command_table_t::calculate_length(1)); // We only need 1 prdt since its only 512 bytes

    auto command_index = slot.first;
    assert(command_index != -1);

    port.region->command_headers[command_index].flags.prdtl = 1; // 1 PRDT entry
    port.region->command_headers[command_index].flags.write = 0; // Reading from device
    port.region->command_headers[command_index].flags.cfl = 5; // h2d fis is 5 dwords long


    auto& fis = *(ahci::regs::h2d_register_fis*)(slot.second->fis);
    fis.type = 0x27;
    fis.flags.c = 1;
    fis.command = packet_device ? commands::identify_packet_interface : commands::identify;
    fis.control = 0x08; // Legacy stuff
    fis.dev_head = 0xA0;

    auto& prdt = slot.second->prdts[0];
    prdt.flags.byte_count = regs::prdt_t::calculate_bytecount(512);
    
    libsigma_phys_region_t region = {};
    if(libsigma_get_phys_region(512, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "Failed to allocate physical region for identification data\n";
        return;
    }

    prdt.low = region.physical_addr & 0xFFFFFFFF;
    prdt.high = (region.physical_addr >> 32) & 0xFFFFFFFF;

    port.wait_ready();

    port.regs->ci |= (1u << command_index); // Start command

    while(port.regs->ci & (1u << command_index)){
        prs_t::is_t is{port.regs->is};
        if(is.tfes){
            prs_t::tfd_t tfd{port.regs->tfd};

            if(tfd.status.error){
                std::cerr << "ahci: Error on identify code: " << tfd.err << std::endl;
                return;
            }
        }
    }

    memcpy((void*)port.identification, (void*)region.virtual_addr, 512);

    auto verify_region_integrity = +[](uint8_t* identity) -> bool {
        if(identity[510] == 0xA5){
            uint8_t checksum = 0;
            for(int i = 0; i < 511; i++)
                checksum += identity[i];

            return ((uint8_t)-checksum == identity[511]);
        } else if(identity[510] != 0){
            return false;
        } else { // Might be a version below ATA-5 which had no checksum
            return true;
        }
    };
    
    assert(verify_region_integrity(port.identification));
    // TODO: Cleanup
}

std::pair<uint32_t, uint32_t> ahci::controller::pi_read_capacity(ahci::controller::port& port){
    auto slot = allocate_command(port, command_table_t::calculate_length(1)); // We only need 1 prdt since its only 512 bytes

    auto command_index = slot.first;
    assert(command_index != -1);

    port.region->command_headers[command_index].flags.prdtl = 1; // 1 PRDT entry
    port.region->command_headers[command_index].flags.write = 0; // Reading from device
    port.region->command_headers[command_index].flags.cfl = 5; // h2d fis is 5 dwords long
    port.region->command_headers[command_index].flags.atapi = 1;


    auto& fis = *(ahci::regs::h2d_register_fis*)(slot.second->fis);
    fis.type = 0x27;
    fis.flags.c = 1;
    fis.command = commands::send_packet;
    fis.control = 0x08; // Legacy stuff
    fis.dev_head = 0xA0;
    fis.features = 0x1;

    auto& prdt = slot.second->prdts[0];
    prdt.flags.byte_count = regs::prdt_t::calculate_bytecount(8);
    
    libsigma_phys_region_t region = {};
    if(libsigma_get_phys_region(8, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "Failed to allocate physical region for SATAPI Read Capacity data\n";
        return {0, 0};
    }

    prdt.low = region.physical_addr & 0xFFFFFFFF;
    prdt.high = (region.physical_addr >> 32) & 0xFFFFFFFF;

    auto& packet = *(commands::packet_commands::read_capacity::packet*)(slot.second->packet);
    packet.command = commands::packet_commands::read_capacity::command;

    port.wait_ready();

    port.regs->ci |= (1u << command_index); // Start command

    while(port.regs->ci & (1u << command_index)){
        prs_t::is_t is{port.regs->is};
        if(is.tfes){
            prs_t::tfd_t tfd{port.regs->tfd};

            if(tfd.status.error){
                std::cerr << "ahci: Error on identify code: " << tfd.err << std::endl;
                return {0, 0};
            }
        }
    }


    auto& response = *(commands::packet_commands::read_capacity::response*)(region.virtual_addr);
    uint32_t lba = response.lba;
    uint32_t block_size = response.block_size;
    return {bswap32(lba), bswap32(block_size)};
}

std::vector<uint8_t> ahci::controller::read_sector(ahci::controller::port& port, uint64_t lba){
    auto slot = allocate_command(port, command_table_t::calculate_length(1)); // Sector size > 4MiB not happening

    auto command_index = slot.first;
    assert(command_index != -1);

    port.region->command_headers[command_index].flags.prdtl = 1; // 1 PRDT entry
    port.region->command_headers[command_index].flags.write = 0; // Reading from device
    port.region->command_headers[command_index].flags.cfl = 5; // h2d fis is 5 dwords long

    auto& fis = *(ahci::regs::h2d_register_fis*)(slot.second->fis);
    fis.type = 0x27;
    fis.flags.c = 1;
    fis.command = commands::read_extended_dma;
    fis.control = 0x08; // Legacy stuff
    fis.dev_head = 0xA0 | (1 << 6); // Legacy stuff + LBA mode
    fis.sector_count_low = 1;
    fis.sector_count_high = 0;
    fis.lba_0 = lba & 0xFF;
    fis.lba_1 = (lba >> 8) & 0xFF;
    fis.lba_2 = (lba >> 16) & 0xFF;
    fis.lba_3 = (lba >> 24) & 0xFF;
    fis.lba_4 = (lba >> 32) & 0xFF;
    fis.lba_5 = (lba >> 40) & 0xFF;    

    auto& prdt = slot.second->prdts[0];
    prdt.flags.byte_count = regs::prdt_t::calculate_bytecount(port.bytes_per_sector);
    
    libsigma_phys_region_t region = {};
    if(libsigma_get_phys_region(port.bytes_per_sector, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "Failed to allocate physical region for sector data\n";
        return {};
    }

    prdt.low = region.physical_addr & 0xFFFFFFFF;
    prdt.high = (region.physical_addr >> 32) & 0xFFFFFFFF;

    port.wait_ready();

    port.regs->ci |= (1u << command_index); // Start command

    while(port.regs->ci & (1u << command_index)){
        prs_t::is_t is{port.regs->is};
        if(is.tfes){
            prs_t::tfd_t tfd{port.regs->tfd};

            if(tfd.status.error){
                std::cerr << "ahci: Error on identify code: " << tfd.err << std::endl;
                return {};
            }
        }
    }

    std::vector<uint8_t> ret{};
    ret.resize(port.bytes_per_sector);

    memcpy((void*)ret.data(), (void*)region.virtual_addr, port.bytes_per_sector);

    return ret;
}