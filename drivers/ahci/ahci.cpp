#include "ahci.hpp"

#include <libsigma/memory.h>
#include <iostream>
#include <sys/mman.h>


ahci::controller::controller(uintptr_t phys_base, size_t size){
    this->base = (regs::hba_t*)libsigma_vm_map(size, nullptr, (void*)phys_base, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON);

    printf("ahci: Initializing controller [%lx -> %lx], virt: %p\n", phys_base, phys_base + size, base);
    printf("ahci: Detected controller, version: [%x]: %d.%d.%d\n", *(uint32_t*)(&base->ghcr.vs), base->ghcr.vs.major, base->ghcr.vs.minor, base->ghcr.vs.patch);
}