#ifndef SIGMA_KERNEL_SMP_CPU
#define SIGMA_KERNEL_SMP_CPU

#include <Sigma/common.h>
#include <Sigma/misc/misc.h>
#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/gdt.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/paging.h>
#include <Sigma/arch/x86_64/cpu.h>

namespace smp::cpu
{
    struct entry;

    C_LINKAGE smp::cpu::entry* get_current_cpu();

    struct entry {
        public:
        entry(): self_ptr((uint64_t)this), lapic_id{0}, gdt{}, tss{}, tss_gdt_offset{0}, features{.raw = 0} {}

        uint64_t self_ptr;

        x86_64::apic::lapic lapic;
        uint32_t lapic_id;

        x86_64::gdt::gdt gdt;
        x86_64::tss::table tss;
        uint16_t tss_gdt_offset;

        x86_64::paging::pcid_cpu_context pcid_context;
        
        x86_64::spinlock::irq_lock irq_lock;

        misc::lazy_initializer<x86_64::kernel_stack> idle_stack;
        misc::lazy_initializer<x86_64::kernel_stack> kstack;

        union {
            struct {
                uint64_t pcid : 1;
                uint64_t invpcid : 1;
                uint64_t smap : 1;
			    uint64_t svm : 1;
                uint64_t x2apic : 1;
                uint64_t vt_d : 1;
            };
            uint64_t raw;
        } features;

        void set_gs(){
            this->self_ptr = (uint64_t)this;

            x86_64::msr::write(x86_64::msr::gs_base, (uint64_t)&self_ptr);
            x86_64::msr::write(x86_64::msr::kernelgs_base, 0);
        }
    };
} // smp::cpu


#endif
