#ifndef SIGMA_KERNEL_SMP_SMP
#define SIGMA_KERNEL_SMP_SMP

#include <Sigma/common.h>
#include <klibc/stdio.h>
#include <klibc/string.h>
#include <Sigma/mm/pmm.h>
#include <Sigma/arch/x86_64/paging.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/types/linked_list.h>
#include <Sigma/smp/ipi.h>

namespace smp
{
    struct cpu_entry {
        uint32_t lapic_id;
        bool x2apic;
        bool bsp;
    };

    C_LINKAGE uint64_t trampoline_start;
    C_LINKAGE uint64_t trampoline_end;
    C_LINKAGE uint8_t trampoline_booted;
    C_LINKAGE uint64_t trampoline_stack;
    C_LINKAGE uint64_t trampoline_paging;

    constexpr uint64_t smp_trampoline_base = 0x1000;

    class multiprocessing {
        public:
        multiprocessing(types::linked_list<cpu_entry>& cpus, x86_64::apic::lapic* lapic);

        private:
        void boot_cpu(cpu_entry& e);

        void boot_external_apic(smp::cpu_entry& cpu);
        void boot_apic(smp::cpu_entry& cpu);

        x86_64::apic::lapic* bsp_lapic;
    };
} // smp


#endif