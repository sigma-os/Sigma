#ifndef SIGMA_ARCH_X86_64_CPU
#define SIGMA_ARCH_X86_64_CPU

#include <Sigma/common.h>
#include <Sigma/misc.h>
#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/smp/cpu.h>

namespace x86_64
{
    void misc_features_init();
        
    namespace pat {
        constexpr uint8_t uncacheable = 0x00;
        constexpr uint8_t write_combining = 0x01;
        constexpr uint8_t write_through = 0x04;
        constexpr uint8_t write_back = 0x06;

        constexpr uint8_t cpuid_bit = 16;

        void init();
        uint64_t get_flags(uint8_t type);
    }

    namespace umip {
        constexpr uint8_t cpuid_bit = 2;
        void init();
    }

    namespace smep {
        constexpr uint8_t cpuid_bit = 7;
        void init();
    }

    namespace smap {
        constexpr uint8_t cpuid_bit = 20;
        void init();

        class smap_guard {
            public:
            smap_guard(){
                if(smp::cpu::entry::get_cpu()->features.smap)
                    asm("clac");
            }
            ~smap_guard(){
                if(smp::cpu::entry::get_cpu()->features.smap)
                    asm("stac");
            }
        };
    }

    namespace pcid {
        constexpr uint8_t pcid_cpuid_bit = 17;
        constexpr uint8_t invpcid_cpuid_bit = 10;

        void init();
    }

    namespace regs {
        union cr4 {
            static cr4 load(){
                uint64_t tmp;
                asm("mov %%cr4, %0" : "=r"(tmp));
                return cr4{tmp};
            }

            static void store(cr4 reg){
                asm("mov %0, %%cr4" : : "r"(reg.raw));
            }

            cr4(uint64_t raw): raw(raw) {}
            cr4(){
                *this = cr4::load();
            }

            void flush(){
                cr4::store(*this);
            }

            struct _bits {
                uint64_t vme : 1;
                uint64_t pvi : 1;
                uint64_t tsd : 1;
                uint64_t de : 1;
                uint64_t pse : 1;
                uint64_t pae : 1;
                uint64_t mce : 1;
                uint64_t pge : 1;
                uint64_t pce : 1;
                uint64_t osfxsr : 1;
                uint64_t osxmmexcpt : 1;
                uint64_t umip : 1;
                uint64_t res_0 : 1;
                uint64_t vmxe : 1;
                uint64_t smxe : 1;
                uint64_t res_1 : 1;
                uint64_t fsgsbase : 1;
                uint64_t pcide : 1;
                uint64_t osxsave : 1;
                uint64_t res_2 : 1;
                uint64_t smep : 1;
                uint64_t smap : 1;
                uint64_t pke : 1;
                uint64_t res_3 : 41;
            };
            static_assert(sizeof(_bits) == 8);
            _bits bits;
            uint64_t raw;
        };
        static_assert(sizeof(cr4) == 8);
    }

    C_LINKAGE void emulate_invpcid(uint64_t kernel_pml4, int pcid);
} // namespace x86_64


#endif