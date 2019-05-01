#ifndef SIGMA_KERNEL_X86_64_APIC
#define SIGMA_KERNEL_X86_64_APIC

#include <Sigma/common.h>
#include <Sigma/interfaces/interrupt_source.h>
#include <Sigma/interfaces/paging_manager.h>

#include <Sigma/arch/x86_64/msr.h>

namespace x86_64::apic
{
    constexpr uint64_t lapic_id = 0x20;
    constexpr uint64_t lapic_version = 0x30;

    constexpr uint64_t lapic_tpr = 0x80;
    constexpr uint64_t lapic_apr = 0x90;
    constexpr uint64_t lapic_ppr = 0xA0;
    constexpr uint64_t lapic_eoi = 0xB0;
    constexpr uint64_t lapic_rrd= 0xC0;
    constexpr uint64_t lapic_ldr = 0xD0;
    constexpr uint64_t lapic_dfr = 0xE0;
    constexpr uint64_t lapic_spurious = 0xF0;

    constexpr uint64_t lapic_icr_low = 0x300;
    constexpr uint64_t lapic_icr_high = 0x310;


    constexpr uint64_t lapic_icr_ds_self = 0x40000;
    constexpr uint64_t lapic_icr_ds_allinc = 0x80000;
    constexpr uint64_t lapic_icr_ds_allex = 0xC0000;
    constexpr uint64_t lapic_icr_tm_level = 0x8000;
    constexpr uint64_t lapic_icr_levelassert = 0x4000;
    constexpr uint64_t lapic_icr_status_pending = 0x1000;
    constexpr uint64_t lapic_icr_dm_logical = 0x800;
    constexpr uint64_t lapic_icr_dm_lowpri = 0x100;
    constexpr uint64_t lapic_icr_dm_smi = 0x200;
    constexpr uint64_t lapic_icr_dm_nmi = 0x400;
    constexpr uint64_t lapic_icr_dm_init = 0x500;
    constexpr uint64_t lapic_icr_dm_sipi = 0x600;

    class lapic {
        public:
        uint32_t read(uint32_t reg){
            uint32_t* val = reinterpret_cast<uint32_t*>(base + KERNEL_VBASE + reg);
            return *val;
        }

        void write(uint32_t reg, uint32_t val){
            uint32_t* data = reinterpret_cast<uint32_t*>(base + KERNEL_VBASE + reg);
            *data = val;
        }

        void send_ipi(uint8_t target_lapic_id, uint32_t flags){
            this->write(x86_64::apic::lapic_icr_high, (target_lapic_id << 24));
            this->write(x86_64::apic::lapic_icr_low, flags);
        
            while((this->read(x86_64::apic::lapic_icr_low) & x86_64::apic::lapic_icr_status_pending));
        }


        lapic(IPaging& paging){
            uint64_t apic_base_msr = msr::read(msr::apic_base);

            base = (apic_base_msr & 0xFFFFFFFFFFFFF000);

            bitops<uint64_t>::bit_set(apic_base_msr, 11); // Set Enable bit
            msr::write(msr::apic_base, apic_base_msr);

            paging.map_page(base, (base + KERNEL_VBASE), map_page_flags_present | map_page_flags_writable | map_page_flags_cache_disable | map_page_flags_no_execute);
        }



        private:
        uint64_t base;
    };
} // x86_64::apic


#endif