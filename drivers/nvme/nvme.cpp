#include "nvme.hpp"
#include <libsigma/memory.h>
#include <iostream>
#include <sys/mman.h>
#include <assert.h>
#include <cstring>
#include <string_view>
#include <libdriver/math.hpp>


nvme::controller::controller(libsigma_resource_region_t region){
    this->base = (volatile regs::bar*)libsigma_vm_map(region.len, nullptr, (void*)region.base, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON);
    
    printf("nvme: Initializing controller, phys: [0x%lx -> 0x%lx], virt: %p\n", region.base, region.base + region.len, base);

    auto vs = regs::bar::vs_t{this->base->vs};

    printf("nvme: Version: %d.%d.%d\n", vs.major, vs.minor, vs.tertiary);

    auto cap = regs::bar::cap_t{this->base->cap};

    printf("nvme: Capabilities: \n");
    this->n_queue_entries = cap.mqes + 1;
    printf("      Max Queue entries: %ld\n", n_queue_entries);
    printf("      Worst case timeout: %d ms\n", cap.to * 500);
    this->doorbell_stride = cap.dstrd;
    printf("      Doorbell stride: %ld bytes\n", pow2(2 + doorbell_stride)); // Bytes = 2 ^ (2 + stride)
    printf("      Memory page size minimum: 0x%lx bytes\n", pow2(12 + cap.mpsmin)); // Stored as 2 ^ (2 + size)
    printf("      Memory page size maximum: 0x%lx bytes\n", pow2(12 + cap.mpsmax)); // Stored as 2 ^ (2 + size)

    printf("      Features: ");
    if(cap.cqr)
        printf("Contiguous Queues Required ");
    if(cap.nssrs)
        printf("NVM Subsystem Reset ");
    if(cap.bps)
        printf("Boot Partition Support ");
    if(cap.pmrs)
        printf("Persistent Memory Region Support ");
    if(cap.cmbs)
        printf("Controller Memory Buffer Support ");
    printf("\n");

    printf("      Arbitration types supported: \n");
    printf("        - Round Robin\n"); // Guaranteed support
    if(cap.ams & (1ull << 0ull)){
        this->weighted_round_robin_supported = true;
        printf("        - Weighted Round Robin with Urgent Priority Class\n");
    } else if(cap.ams & (1ull << 1ull)) {
        printf("        - Vendor Specific\n");
    }

    printf("      Command sets supported: \n");
    if(cap.css & (1ull << 0ull))
        printf("        - NVM command set\n");
    if(cap.css & (1ull << 7ull))
        printf("        - No I/O Command set supported\n");

    this->reset_subsystem();

    auto cc = regs::bar::cc_t{this->base->cc};
    cc.en = 0;
    this->base->cc = cc.raw;

    while(this->base->controller_status & (1 << 0)); // Wait for CSTS.RDY to become 0

    qid_t admin_qid = 0; // Admin queue
    uint16_t* admin_submission_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + (2 * admin_qid * (4 << this->doorbell_stride)));
    uint16_t* admin_completion_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + ((2 * admin_qid + 1) * (4 << this->doorbell_stride)));
    this->admin_queue = queue_pair{n_queue_entries, admin_submission_doorbell, admin_completion_doorbell, admin_qid};

    this->base->acq = this->admin_queue.get_completion_phys_base();
    this->base->asq = this->admin_queue.get_submission_phys_base();
    
    auto aqa = regs::bar::aqa_t{this->base->aqa};
    aqa.acqs = admin_queue.get_n_entries() - 1;
    aqa.asqs = admin_queue.get_n_entries() - 1;

    this->base->aqa = aqa.raw;

    if(this->weighted_round_robin_supported)
        cc.ams |= 1; // If supported use Weighted Round robin, otherwise just use the normal one

    cc.mps = 0; // MPS = 0 -> Page size = 0x1000, Page size = 2 ^ (12 + MPS)
    cc.css = 0; // CSS = 0 -> NVM command set
    cc.iocqes = 4; // Set I/O Completion queue entry size, 2 ^ 6 = 64 bytes
    cc.iosqes = 6; // Set I/O Submission queue entry size 2 ^ 4 = 16 bytes
    cc.en = 1; // Set Enable

    this->base->cc = cc.raw;

    while(!(this->base->controller_status & (1 << 0))); // Wait for CSTS.RDY to become 1

    std::cerr << "nvme: Enabled controller\n";

    std::cout << "nvme: Identifying...";

    regs::identify_info info{};
    this->identify(&info);

    std::cout << "Done\n";
    this->print_identify_info(info);

    // Let the controller allocate as many queues as possible so we don't have to care about it
    this->set_features(regs::n_queues_fid, ~0);

    qid_t io_qid = 1; // IO queue
    uint16_t* io_submission_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + (2 * io_qid * (4 << this->doorbell_stride)));
    uint16_t* io_completion_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + ((2 * io_qid + 1) * (4 << this->doorbell_stride)));
    auto* io_pair = new queue_pair{n_queue_entries, io_submission_doorbell, io_completion_doorbell, io_qid};

    if(!this->register_queue_pair(*io_pair)){
        std::cerr << "nvme: Failed to create I/O queue\n";
        return;
    }
}

void nvme::controller::set_power_state(nvme::controller::shutdown_types type){
    switch (type)
    {
    case shutdown_types::AbruptShutdown:
        this->base->cc &= ~(3 << 14); // Clear bits
        this->base->cc |= (1 << 15); // Set abrupt shutdown bit
        break;
    case shutdown_types::NormalShutdown:
        this->base->cc &= ~(3 << 14); // Clear bits
        this->base->cc |= (1 << 14); // Set normal shutdown bit
        break;
    }
}

void nvme::controller::reset_subsystem(){
    auto cap = regs::bar::cap_t{this->base->cap};
    if(cap.nssrs)
        this->base->subsystem_reset = 0x4E564D65;
}

bool nvme::controller::identify(nvme::regs::identify_info* info){
    regs::identify_command cmd{};

    cmd.header.opcode = regs::identify_opcode;
    cmd.header.namespace_id = 0;
    cmd.cns = 1;

    libsigma_phys_region_t region = {};
    if(libsigma_get_phys_region(0x1000, PROT_READ | PROT_WRITE, MAP_ANON, &region)){
        std::cerr << "nvme: Failed to allocate physical region for identify\n";
        return false;
    }

    cmd.header.prp1 = region.physical_addr;

    this->admin_queue.send_and_wait((regs::command*)&cmd);

    auto* buf = (regs::identify_info*)region.virtual_addr;

    memcpy(info, buf, 0x1000);
    return true;
}

bool nvme::controller::set_features(uint8_t fid, uint32_t data){
    regs::set_features_command cmd{};

    cmd.header.opcode = regs::set_features_opcode;
    cmd.header.namespace_id = 0;
    cmd.fid = fid;
    cmd.data = data;
    
    auto status = this->admin_queue.send_and_wait((regs::command*)&cmd);
    if(status != 0){
        return false;
    }
    
    return true;
}

bool nvme::controller::register_queue_pair(nvme::queue_pair& pair){
    regs::create_completion_queue_command cq_cmd{};
    cq_cmd.header.opcode = regs::create_completion_queue_opcode;
    cq_cmd.header.prp1 = pair.get_completion_phys_base();
    cq_cmd.qid = pair.get_qid();
    cq_cmd.size = pair.get_n_entries() - 1;
    cq_cmd.irq_enable = 0;
    cq_cmd.pc = 1;
    if(!this->admin_queue.send_and_wait((regs::command*)&cq_cmd))
        return false;

    regs::create_submission_queue_command sq_cmd{};
    sq_cmd.header.opcode = regs::create_submission_queue_opcode;
    sq_cmd.header.prp1 = pair.get_submission_phys_base();
    sq_cmd.qid = pair.get_qid();
    sq_cmd.cqid = pair.get_qid();
    sq_cmd.size = pair.get_n_entries() - 1;
    sq_cmd.pc = 1;
    sq_cmd.priority = 2; // Medium Priority
    if(!this->admin_queue.send_and_wait((regs::command*)&sq_cmd))
        return false;
    
    return true;
}

void nvme::controller::print_identify_info(nvme::regs::identify_info& info){
    std::cout << "nvme: Identification info" << std::endl;
    std::cout << "      PCI Vendor id: " << std::hex << info.pci_vendor_id << ", Subsystem Vendor ID: " << info.pci_subsystem_vendor_id << std::endl;
    std::cout << "      Model: " << std::string_view{info.model_number, sizeof(info.model_number)} << std::endl;
    std::cout << "      Serial Number: " << std::string_view{info.serial_number, sizeof(info.serial_number)} << std::endl;
    std::cout << "      Firmware Revision: " << std::string_view{info.fw_revision, sizeof(info.fw_revision)} << std::endl;
    // See http://standards-oui.ieee.org/oui.txt
    printf("      Organizationally Unique Identifier: %02lX%02lX%02lX\n", (uint64_t)info.ieee_oui_id[0] & 0xFF, (uint64_t)info.ieee_oui_id[1] & 0xFF, (uint64_t)info.ieee_oui_id[2] & 0xFF);
    if(strlen(info.subnqn))
        std::cout << "      NVM Subsystem Name: " << (const char*)info.subnqn << std::endl;

    std::cout << "      Controller ID: " << info.controller_id << std::endl;
    if(info.cntrltype){
        std::cout << "      Controller type: ";
        switch (info.cntrltype)
        {
        case 1:
            std::cout << "I/O Controller";
            break;
    
        case 2:
            std::cout << "Discovery Controller";
            break;
    
        case 3:
            std::cout << "Administrative Controller";
            break;

        default:
        case 0:
            std::cout << "Reserved";
            break;
        }

        std::cout << std::endl;
    }

    if(info.oacs){
        std::cout << "      Optional Admin Command Support:" << std::endl;

        if(info.oacs & (1 << 0))
            std::cout << "       - Security {Send, Receive}" << std::endl;
        if(info.oacs & (1 << 1))
            std::cout << "       - Format NVM" << std::endl;
        if(info.oacs & (1 << 2))
            std::cout << "       - Firmware {Commit, Image Download}" << std::endl;
        if(info.oacs & (1 << 3))
            std::cout << "       - NS management" << std::endl;
        if(info.oacs & (1 << 4))
            std::cout << "       - Device Self-test" << std::endl;
        if(info.oacs & (1 << 5))
            std::cout << "       - Directives" << std::endl;
        if(info.oacs & (1 << 6))
            std::cout << "       - NVMe-MI {Send, Receive}" << std::endl;
        if(info.oacs & (1 << 7))
            std::cout << "       - Virtualization Management" << std::endl;
        if(info.oacs & (1 << 8))
            std::cout << "       - Doorbell Buffer Config" << std::endl;
        if(info.oacs & (1 << 9))
            std::cout << "       - Get LBA Status" << std::endl;
    }

    if(info.oncs){
        std::cout << "      Optional NVM Command Support" << std::endl;

        if(info.oncs & (1 << 0))
            std::cout << "       - Compare" << std::endl;
        if(info.oncs & (1 << 1))
            std::cout << "       - Write Uncorrectable" << std::endl;
        if(info.oncs & (1 << 2))
            std::cout << "       - Dataset Management" << std::endl;
        if(info.oncs & (1 << 3))
            std::cout << "       - Write Zeroes" << std::endl;
        if(info.oncs & (1 << 4))
            std::cout << "       - Non-zero {Save, Select} field in Get Features" << std::endl;
        if(info.oncs & (1 << 5))
            std::cout << "       - Reservations" << std::endl;
        if(info.oncs & (1 << 6))
            std::cout << "       - Timestamp" << std::endl;
        if(info.oncs & (1 << 7))
            std::cout << "       - Verify" << std::endl;
    }

    if(info.fuses){
        std::cout << "      Supported Fused Commands" << std::endl;

        if(info.fuses & (1 << 0))
            std::cout << "       - Compare and Write" << std::endl;
    }

    
    std::cout << "      Recommended Arbitration Burst: 0x" << std::hex << pow2(info.rab) << std::endl;

    auto cap = regs::bar::cap_t{this->base->cap};
    if(info.mtds)
        std::cout << "      Maximum DMA Transfer Size: 0x" << std::hex << (info.mtds * pow2(12 + cap.mpsmin)) << std::endl;
    else
        std::cout << "      Maximum DMA Transfer Size: Infinity" << std::endl;

    if(info.rtd3r)
        std::cout << "      D3 power state -> Normal Latency " << info.rtd3r << " microseconds\n";

    if(info.rtd3e)
        std::cout << "      Normal -> D3 power state Latency " << info.rtd3e << " microseconds\n";    

    std::cout << "      Submission Queue Entry Size: Minimum: " << std::dec << pow2(info.sqes & 0xF) << " Maximum: " << pow2((info.sqes >> 4) & 0xF) << std::endl;
    std::cout << "      Completion Queue Entry Size: Minimum: " << std::dec << pow2(info.cqes & 0xF) << " Maximum: " << pow2((info.cqes >> 4) & 0xF) << std::endl;

    std::cout << "      Abort Command Limit: " << info.acl + 1 << std::endl;
    if(info.maxcmd)
        std::cout << "      Maximum Commands per Queue: " << info.maxcmd << std::endl;

    std::cout << "      Number of Namespaces: " << info.nn << std::endl;

    if(info.mnan)
        std::cout << "      Maximum number of NSes in NVM subsystem: " << info.mnan << std::endl;
    else
        std::cout << "      Maximum number of NSes in NVM subsystem: " << info.nn << std::endl;
}