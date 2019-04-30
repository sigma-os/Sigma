#ifndef SIGMA_KERNEL_SMP_SMP
#define SIGMA_KERNEL_SMP_SMP

#include <Sigma/common.h>
#include <klibc/stdio.h>

#include <Sigma/types/linked_list.h>

namespace smp
{
    struct cpu_entry {
        public:
        cpu_entry(): lapic_id(0), lapic_version(0), bsp(false), on_chip_apic(false), cpu_signature_stepping(0), cpu_signature_model(0), cpu_signature_family(0), alignment(0) {}
        cpu_entry(uint8_t lapic_id, uint8_t lapic_version, bool bsp, bool on_chip_apic, uint8_t cpu_signature_stepping, uint8_t cpu_signature_model, uint8_t cpu_signature_family): lapic_id(lapic_id), lapic_version(lapic_version), bsp(bsp), on_chip_apic(on_chip_apic), cpu_signature_stepping(cpu_signature_stepping), cpu_signature_model(cpu_signature_model), cpu_signature_family(cpu_signature_family), alignment(0) {}

        uint8_t lapic_id;
        uint8_t lapic_version;

        bool bsp;
        bool on_chip_apic;

        uint8_t cpu_signature_stepping : 4;
        uint8_t cpu_signature_model : 4;
        uint8_t cpu_signature_family : 4;

        uint8_t alignment : 4;
    };

    class multiprocessing {
        public:
        multiprocessing(types::linked_list<cpu_entry>& cpus){
            for(auto& e : cpus){
                printf("%x\n", e.lapic_id);
            }
        }
    };
} // smp


#endif