#ifndef SIGMA_KERNEL_SMP_CPU
#define SIGMA_KERNEL_SMP_CPU

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/tss.h>
#include <Sigma/arch/x86_64/drivers/apic.h>

namespace smp::cpu
{
    struct entry {
        public:
        entry(): lapic(x86_64::apic::lapic()), lapic_id(0), tss(nullptr), gs_pointer(nullptr){}
        ~entry(){
            if(gs_pointer != nullptr) delete gs_pointer;
        }
        x86_64::apic::lapic lapic;
        uint8_t lapic_id;
        x86_64::tss::table* tss;
        uint64_t* gs_pointer;

        void set_gs(){
            if(gs_pointer != nullptr) delete gs_pointer;

            gs_pointer = new uint64_t;

            *gs_pointer = reinterpret_cast<uint64_t>(this);


            x86_64::msr::write(x86_64::msr::kernelgs_base, reinterpret_cast<uint64_t>(gs_pointer));
            x86_64::msr::write(x86_64::msr::gs_base, 0);
            asm("swapgs");
        }
    };


    C_LINKAGE smp::cpu::entry* get_current_cpu();
} // smp::cpu


#endif