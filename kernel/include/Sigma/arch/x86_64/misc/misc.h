#ifndef SIGMA_ARCH_X86_64_CPUID
#define SIGMA_ARCH_X86_64_CPUID

#include <Sigma/common.h>
#include <cpuid.h>

namespace x86_64
{
    bool cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);
    bool cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx);
    uint64_t read_tsc();
    void identify_cpu();

    C_LINKAGE uint64_t read_xcr(uint32_t xcr);
    C_LINKAGE void write_xcr(uint32_t xcr, uint64_t value);

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
        constexpr uint64_t MMXEXT = 0x00400000;
        constexpr uint64_t LM = 0x20000000;
        constexpr uint64_t n3DNOWP = 0x40000000;
        constexpr uint64_t n3DNOW = 0x80000000;

        /* Features in %ebx for leaf 0x80000008 */
        constexpr uint64_t CLZERO = 0x00000001;
        constexpr uint64_t WBNOINVD = 0x00000200;
    }
} // namespace x86_64


#endif