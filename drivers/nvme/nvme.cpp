#include "nvme.hpp"
#include <libsigma/memory.h>
#include <iostream>
#include <sys/mman.h>
#include <assert.h>
#include <cstring>
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

    qid_t qid = 0; // Admin queue
    uint16_t* submission_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + (2 * qid * (4 << this->doorbell_stride)));
    uint16_t* completion_doorbell = (uint16_t*)((size_t)this->base + 0x1000 + ((2 * qid + 1) * (4 << this->doorbell_stride)));
    this->admin_queue = queue_pair{n_queue_entries, submission_doorbell, completion_doorbell, qid};

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