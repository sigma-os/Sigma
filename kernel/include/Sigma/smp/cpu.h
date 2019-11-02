#ifndef SIGMA_KERNEL_SMP_CPU
#define SIGMA_KERNEL_SMP_CPU

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/paging.h>

namespace smp::cpu
{
    struct entry;

    C_LINKAGE smp::cpu::entry* get_current_cpu();

    struct entry {
        public:
        entry(): lapic(x86_64::apic::lapic()), lapic_id(0), tss(nullptr), pcid_context({}), self_ptr(reinterpret_cast<uint64_t>(this)){}

        x86_64::apic::lapic lapic;
        uint8_t lapic_id;
        x86_64::tss::table* tss;
        x86_64::paging::pcid_cpu_context pcid_context;
        uint64_t self_ptr;

        struct {
            uint64_t pcid : 1;
            uint64_t invpcid : 1;
            uint64_t smap : 1;
        } features;

        void set_gs(){
            self_ptr = reinterpret_cast<uint64_t>(this);


            x86_64::msr::write(x86_64::msr::kernelgs_base, reinterpret_cast<uint64_t>(&self_ptr));
            x86_64::msr::write(x86_64::msr::gs_base, 0);
            asm("swapgs");
        }

        static entry* get_cpu(){
            return cpu::get_current_cpu();
        }
    };
} // smp::cpu


#endif