#include "ahci.hpp"

#include <libsigma/memory.h>
#include <iostream>
#include <sys/mman.h>
#include <assert.h>

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

std::pair<int, ahci::regs::h2d_register_fis*> ahci::controller::allocate_command(ahci::controller::port& port, size_t fis_size){
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

    //assert((fis_size % sizeof(uint32_t)) == 0); // Make sure FIS size is divisible by 4

    header.flags.cfl = 5; // h2d fis is 5 dwords long

    return {index, (ahci::regs::h2d_register_fis*)region.virtual_addr};
}

int ahci::controller::get_free_command_slot(ahci::controller::port& port){
    for(int i = 0; i < n_command_slots; i++)
        if((port.regs->ci & (1u << i)) == 0)
            return i;
                    
    return -1;
}