#ifndef SIGMA_ARCH_X86_64_GENERAL
#define SIGMA_ARCH_X86_64_GENERAL

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/msr.h>

namespace x86_64
{
    void misc_features_init();
    
    namespace umip {
        constexpr uint8_t cpuid_bit = 2;
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

            struct {
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
                uint64_t pcide : 1;
                uint64_t osxsave : 1;
                uint64_t res_2 : 1;
                uint64_t smep : 1;
                uint64_t smap : 1;
                uint64_t res_3 : 41;
            };
            uint64_t raw;
        };
        static_assert(sizeof(cr4) == 8);
    }
} // namespace x86_64


#endif