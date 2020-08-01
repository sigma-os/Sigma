#ifndef SIGMA_ARCH_X86_64_CPU
#define SIGMA_ARCH_X86_64_CPU

#include <Sigma/common.h>
#include <Sigma/misc/misc.h>
#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/arch/x86_64/misc/misc.h>

#include <klibc/string.h>
#include <klibcxx/algorithm.hpp>

namespace x86_64
{
    void misc_early_features_init();
    void misc_bsp_late_features_init();
    void identify_cpu();
        
    namespace pat {
        constexpr uint8_t uncacheable = 0x00;
        constexpr uint8_t write_combining = 0x01;
        constexpr uint8_t write_through = 0x04;
        constexpr uint8_t write_back = 0x06;

        constexpr uint64_t sigma_pat = write_back | (write_combining << 8) | (write_through << 16) | (uncacheable << 24);
        constexpr uint64_t default_pat = 0x0007040600070406ull;

        void init();
        uint64_t get_flags(uint8_t type);
    }

    namespace umip {
        void init();
    }

    namespace smep {
        void init();
    }

    namespace smap {
        void init();

        class smap_guard {
            public:
            smap_guard();
            ~smap_guard();
        };
    }

    namespace pcid {
        void init();
    }

    namespace tsd {
        void init();
    }

    namespace tme {
        void init();
        void restore_key();
    }

    class kernel_stack {
        public:
        kernel_stack(size_t size = kernel_stack::default_size): _size{size} {
            this->_bottom = new uint8_t[this->_size];
            memset(this->_bottom, 0, this->_size);

            this->_top = this->_bottom + this->_size;
        }

        ~kernel_stack(){
            if(this->_bottom)
                delete[] this->_bottom;
        }

        kernel_stack(const kernel_stack&) = delete;
        kernel_stack(kernel_stack&& b){
            this->~kernel_stack();

            this->_size = std::move(b._size);
            this->_bottom = std::move(b._bottom);
            this->_top = std::move(b._top);
        }

        kernel_stack& operator=(kernel_stack other) = delete;

        void* top(){
            return this->_top;
        }

        void* bottom(){
            return this->_bottom;
        }

        size_t size(){
            return this->_size;
        }

        void reset(){
            memset(this->_bottom, 0, this->_size);
            this->_top = this->_bottom + this->_size;
        }

        template<typename T, typename... Args>
        T* push(Args&&... args){
            this->_top -= ALIGN_UP(sizeof(T), ((16 < alignof(T)) ? alignof(T) : 16)); // Align to at the very least ABI requirements
            return new (this->_top) T{std::forward<Args>(args)...};
        }

        private:
        size_t _size;
        uint8_t* _bottom, *_top;

        constexpr static size_t default_size = 0x8000;  
    };
    
    namespace regs {
        union cr4 {
            void load(){
                uint64_t tmp;
                asm("mov %%cr4, %0" : "=r"(tmp));
                this->raw = tmp;
            }

            void store(){
                asm("mov %0, %%cr4" : : "r"(this->raw));
            }

            explicit cr4(uint64_t raw): raw(raw) {}
            cr4(){ load(); }

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
                uint64_t la57 : 1;
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

        union xcr0 {
            void load(){
                this->raw = x86_64::read_xcr(0);
            }

            void store(){
                x86_64::write_xcr(0, this->raw);
            }

            explicit xcr0(uint64_t raw): raw(raw) {}
            xcr0(){ this->load(); }

            struct _bits {
                uint64_t fpu_mmx : 1;
                uint64_t sse : 1;
                uint64_t avx : 1;
                uint64_t bndreg : 1;
                uint64_t bndcsr : 1;
                uint64_t opmask : 1;
                uint64_t zmm_hi256 : 1;
                uint64_t zmm_hi16 : 1;
                uint64_t pkru : 1;
            };
            static_assert(sizeof(_bits) == 8);
            _bits bits;
            uint64_t raw;
        };
        static_assert(sizeof(xcr0) == 8);
    }



    #pragma region cpuid bits
    namespace cpuid_bits {
        /* Features in %ecx for leaf 1 */
        constexpr uint64_t SSE3 = 0x00000001;
        constexpr uint64_t PCLMULQDQ = 0x00000002;
        constexpr uint64_t PCLMUL = PCLMULQDQ;/* for gcc compat */
        constexpr uint64_t DTES64 = 0x00000004;
        constexpr uint64_t MONITOR = 0x00000008;
        constexpr uint64_t DSCPL = 0x00000010;
        constexpr uint64_t VMX = 0x00000020;
        constexpr uint64_t SMX = 0x00000040;
        constexpr uint64_t EIST = 0x00000080;
        constexpr uint64_t TM2 = 0x00000100;
        constexpr uint64_t SSSE3 = 0x00000200;
        constexpr uint64_t CNXTID = 0x00000400;
        constexpr uint64_t SDBG = 0x00000800;
        constexpr uint64_t FMA = 0x00001000;
        constexpr uint64_t CMPXCHG16B = 0x00002000;
        constexpr uint64_t xTPR = 0x00004000;
        constexpr uint64_t PDCM = 0x00008000;
        constexpr uint64_t PCID = 0x00020000;
        constexpr uint64_t DCA = 0x00040000;
        constexpr uint64_t SSE41 = 0x00080000;
        constexpr uint64_t SSE4_1 = SSE41; /* for gcc compat */
        constexpr uint64_t SSE42 = 0x00100000;
        constexpr uint64_t SSE4_2 = SSE42; /* for gcc compat */
        constexpr uint64_t x2APIC = 0x00200000;
        constexpr uint64_t MOVBE = 0x00400000;
        constexpr uint64_t POPCNT = 0x00800000;
        constexpr uint64_t TSCDeadline = 0x01000000;
        constexpr uint64_t AESNI = 0x02000000;
        constexpr uint64_t AES = AESNI; /* for gcc compat */
        constexpr uint64_t XSAVE = 0x04000000;
        constexpr uint64_t OSXSAVE = 0x08000000;
        constexpr uint64_t AVX = 0x10000000;
        constexpr uint64_t F16C = 0x20000000;
        constexpr uint64_t RDRND = 0x40000000;

        /* Features in %edx for leaf 1 */
        constexpr uint64_t FPU = 0x00000001;
        constexpr uint64_t VME = 0x00000002;
        constexpr uint64_t DE = 0x00000004;
        constexpr uint64_t PSE = 0x00000008;
        constexpr uint64_t TSC = 0x00000010;
        constexpr uint64_t MSR = 0x00000020;
        constexpr uint64_t PAE = 0x00000040;
        constexpr uint64_t MCE = 0x00000080;
        constexpr uint64_t CX8 = 0x00000100;
        constexpr uint64_t CMPXCHG8B = CX8; /* for gcc compat */
        constexpr uint64_t APIC = 0x00000200;
        constexpr uint64_t SEP = 0x00000800;
        constexpr uint64_t MTRR = 0x00001000;
        constexpr uint64_t PGE = 0x00002000;
        constexpr uint64_t MCA = 0x00004000;
        constexpr uint64_t CMOV = 0x00008000;
        constexpr uint64_t PAT = 0x00010000;
        constexpr uint64_t PSE36 = 0x00020000;
        constexpr uint64_t PSN = 0x00040000;
        constexpr uint64_t CLFSH = 0x00080000;
        constexpr uint64_t DS = 0x00200000;
        constexpr uint64_t ACPI = 0x00400000;
        constexpr uint64_t MMX = 0x00800000;
        constexpr uint64_t FXSR = 0x01000000;
        constexpr uint64_t FXSAVE = FXSR; /* for gcc compat */
        constexpr uint64_t SSE = 0x02000000;
        constexpr uint64_t SSE2 = 0x04000000;
        constexpr uint64_t SS = 0x08000000;
        constexpr uint64_t HTT = 0x10000000;
        constexpr uint64_t TM = 0x20000000;
        constexpr uint64_t IA64 = 0x40000000;
        constexpr uint64_t PBE = 0x80000000;

        /* Features in %ebx for leaf 7 sub-leaf 0 */
        constexpr uint64_t FSGSBASE = 0x00000001;
        constexpr uint64_t IA32_TSC_ADJUST = 0x2;
        constexpr uint64_t SGX = 0x00000004;
        constexpr uint64_t BMI1 = 0x00000008;
        constexpr uint64_t HLE = 0x00000010;
        constexpr uint64_t AVX2 = 0x00000020;
        constexpr uint64_t SMEP = 0x00000080;
        constexpr uint64_t BMI2 = 0x00000100;
        constexpr uint64_t ENH_MOVSB = 0x00000200;
        constexpr uint64_t INVPCID = 0x00000400;
        constexpr uint64_t RTM = 0x00000800;
        constexpr uint64_t MPX = 0x00004000;
        constexpr uint64_t AVX512F = 0x00010000;
        constexpr uint64_t AVX512DQ = 0x00020000;
        constexpr uint64_t RDSEED = 0x00040000;
        constexpr uint64_t ADX = 0x00080000;
        constexpr uint64_t SMAP = 0x100000;
        constexpr uint64_t AVX512IFMA = 0x00200000;
        constexpr uint64_t CLFLUSHOPT = 0x00800000;
        constexpr uint64_t CLWB = 0x01000000;
        constexpr uint64_t AVX512PF = 0x04000000;
        constexpr uint64_t AVX512ER = 0x08000000;
        constexpr uint64_t AVX512CD = 0x10000000;
        constexpr uint64_t SHA = 0x20000000;
        constexpr uint64_t AVX512BW = 0x40000000;
        constexpr uint64_t AVX512VL = 0x80000000;

        /* Features in %ecx for leaf 7 sub-leaf 0 */
        constexpr uint64_t PREFTCHWT1 = 0x00000001;
        constexpr uint64_t AVX512VBMI = 0x00000002;
        constexpr uint64_t UMIP = 0x4;
        constexpr uint64_t PKU = 0x00000008;
        constexpr uint64_t OSPKE = 0x00000010;
        constexpr uint64_t WAITPKG = 0x00000020;
        constexpr uint64_t AVX512VBMI2 = 0x00000040;
        constexpr uint64_t SHSTK = 0x00000080;
        constexpr uint64_t GFNI = 0x00000100;
        constexpr uint64_t VAES = 0x00000200;
        constexpr uint64_t VPCLMULQDQ = 0x00000400;
        constexpr uint64_t AVX512VNNI = 0x00000800;
        constexpr uint64_t AVX512BITALG = 0x00001000;
        constexpr uint64_t TME = 0x2000;
        constexpr uint64_t AVX512VPOPCNTDQ = 0x00004000;
        constexpr uint64_t RDPID = 0x00400000;
        constexpr uint64_t CLDEMOTE = 0x02000000;
        constexpr uint64_t MOVDIRI = 0x08000000;
        constexpr uint64_t MOVDIR64B = 0x10000000;

        /* Features in %edx for leaf 7 sub-leaf 0 */
        constexpr uint64_t AVX5124VNNIW = 0x00000004;
        constexpr uint64_t AVX5124FMAPS = 0x00000008;
        constexpr uint64_t PCONFIG = 0x00040000;
        constexpr uint64_t IBT = 0x00100000;

        /* Features in %eax for leaf 13 sub-leaf 1 */
        constexpr uint64_t XSAVEOPT = 0x00000001;
        constexpr uint64_t XSAVEC = 0x00000002;
        constexpr uint64_t XSAVES = 0x00000008;

        /* Features in %eax for leaf 0x14 sub-leaf 0 */
        constexpr uint64_t PTWRITE = 0x00000010;

        /* Features in %ecx for leaf 0x80000001 */
        constexpr uint64_t LAHF_LM = 0x00000001;
        constexpr uint64_t SVM = 0x00000004;
        constexpr uint64_t ABM = 0x00000020;
        constexpr uint64_t LZCNT = ABM; /* for gcc compat */
        constexpr uint64_t SSE4a = 0x00000040;
        constexpr uint64_t PRFCHW = 0x00000100;
        constexpr uint64_t XOP = 0x00000800;
        constexpr uint64_t LWP = 0x00008000;
        constexpr uint64_t FMA4 = 0x00010000;
        constexpr uint64_t TBM = 0x00200000;
        constexpr uint64_t MWAITX = 0x20000000;

        /* Features in %edx for leaf 0x80000001 */
        constexpr uint64_t SYSCALL = 0x800;
        constexpr uint64_t NX = 0x100000;
        constexpr uint64_t MMXEXT = 0x00400000;
        constexpr uint64_t FFXSR = 0x2000000;
        constexpr uint64_t PDPE1GB = 0x4000000;
        constexpr uint64_t RDTSCP = 0x8000000;
        constexpr uint64_t LM = 0x20000000;
        constexpr uint64_t n3DNOWP = 0x40000000;
        constexpr uint64_t n3DNOW = 0x80000000;
                                      

        /* Features in %ebx for leaf 0x80000008 */
        constexpr uint64_t CLZERO = 0x00000001;
        constexpr uint64_t WBNOINVD = 0x00000200;
    }
    #pragma endregion

    C_LINKAGE void emulate_invpcid(uint64_t kernel_pml4, int pcid);
} // namespace x86_64


#endif