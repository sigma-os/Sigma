#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/smp/cpu.h>
#include <klibcxx/mutex.hpp>
#include <klibc/stdio.h>

bool x86_64::cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d){
    return __get_cpuid(leaf, &eax, &ebx, &c, &d);
}

bool x86_64::cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& c, uint32_t& d){
    return __get_cpuid_count(leaf, subleaf, &eax, &ebx, &c, &d);
}

uint64_t x86_64::read_tsc(){
    uint64_t tsc_low = 0;
    uint64_t tsc_high = 0;
    asm ("rdtsc" : "=a"(tsc_low), "=d"(tsc_high));
    return (tsc_low | (tsc_high << 32));
}

static void identify_intel(uint16_t family_id, uint8_t model_id, uint8_t stepping){
    auto identify_family_4 = [&](){
        debug_printf("    Family: Intel 486\n");
        switch (model_id)
        {
        case 0:
            debug_printf("    Model: Intel i80486DX-25/33\n");
            break;
        case 1:
            debug_printf("    Model: Intel i80486DX-50\n");
            break;
        case 2:
            debug_printf("    Model: Intel i80486SX\n");
            break;
        case 3:
            debug_printf("    Model: Intel i80486DX2\n");
            break;
        case 4:
            debug_printf("    Model: Intel i80486SL\n");
            break;
        case 5:
            debug_printf("    Model: Intel i80486SX2\n");
            break;
        case 7:
            debug_printf("    Model: Intel i80486DX2WB\n");
            break;
        case 8:
            debug_printf("    Model: Intel i80486DX4\n");
            break;
        case 9:
            debug_printf("    Model: Intel i80486DX4WB\n");
            break;
        default:
            debug_printf("    Model: Unknown [%x]\n", model_id);
            break;
        }

        debug_printf("    Stepping: %x\n", stepping);
    };

    auto identify_family_5 = [&](){
        debug_printf("    Family: Intel P5\n");
        switch (model_id)
        {
        case 0:
            debug_printf("    Model: P5 A-step\n");
            break;
        case 1:
            debug_printf("    Model: P5\n");
            break;
        case 2:
            debug_printf("    Model: P54C\n");
            break;
        case 3:
            debug_printf("    Model: P24T Overdrive\n");
            break;
        case 4:
            debug_printf("    Model: P54C\n");
            break;

        case 7:
            debug_printf("    Model: P54C\n");
            break;

        case 8:
            debug_printf("    Model: P55C (0,25 micrometer)\n");
            break;
        default:
            debug_printf("    Model: Unknown [%x]\n", model_id);
            break;
        }
        debug_printf("    Stepping: %x\n", stepping);
    };

    auto identify_family_6 = [&](){
        debug_printf("    Family: Intel P6 and newer\n");
        switch (model_id)
        {
        case 0:
            debug_printf("    Model: P6 A-step\n");
            break;
        case 1:
            debug_printf("    Model: P6\n");
            break;
        case 3:
            debug_printf("    Model: P2 (0,28 micrometer)\n");
            break;
        case 5:
            debug_printf("    Model: P2 (0,25 micrometer)\n");
            break;
        case 6:
            debug_printf("    Model: P2 with on-die L2 cache\n");
            break;
        case 7:
            debug_printf("    Model: P3 (0.25 micrometer)\n");
            break;
        case 8:
            debug_printf("    Model: P3 (0.18 micrometer) with 256 KB on-die L2\n");
            break;
        case 0xA:
            debug_printf("    Model: P3 (0.18 micrometer) with 2 MB on-die L2\n");
            break;
        case 0x7E:
            debug_printf("    Model: Ice Lake U/Y\n");
            break;
        case 0x4E:
            debug_printf("    Model: Skylake U/Y\n");
            break;
        case 0x5E:
            debug_printf("    Model: Skylake DT/H/S\n");
            break;
        case 0x8E:
            switch (stepping)
            {
            case 0x9:
                debug_printf("    Model: Kaby Lake Y/U/R\n");
                break;
            case 0xA:
                debug_printf("    Model: Coffee Lake U\n");
                break;
            default:
                debug_printf("    Model: Unknown [Model: %x] [Stepping: %x]\n", model_id, stepping);
                break;
            }
            break;
        case 0x9E:
            switch (stepping)
            {
            case 0x9:
                debug_printf("    Model: Kaby Lake DT/H/S/X\n");
                break;
            case 0xA:
                debug_printf("    Model: Coffee Lake S/H\n");
                break;
            default:
                debug_printf("    Model: Unknown [Model: %x] [Stepping: %x]\n", model_id, stepping);
                break;
            }
            break;
        default:
            debug_printf("    Model: Unknown [%x]\n", model_id);
            break;
        }
        debug_printf("    Stepping: %x\n", stepping);
    };


    switch (family_id)
    {
    case 4: 
        identify_family_4();
        break;
    case 5:
        identify_family_5();
        break;
    case 6:
        identify_family_6();
        break;
    default:
        debug_printf("    Family: Unknown [%x]\n", family_id);
        break;
    }
}

x86_64::spinlock::mutex identify_mutex{};

void x86_64::identify_cpu(){
    std::lock_guard guard{identify_mutex};
    debug_printf("[CPU]: Detecting CPU with id: %d\n", smp::cpu::get_current_cpu()->lapic_id);

    uint32_t a = 0, b = 0, c = 0, d = 0;
    x86_64::cpuid(0, a, b, c, d);

    char vendorID[13] = "";
    *((uint32_t*)vendorID) = b;
    *((uint32_t*)vendorID + 1) = d;
    *((uint32_t*)vendorID + 2) = c;

    debug_printf("    VendorID: %s\n", vendorID);

    x86_64::cpuid(1, a, b, c, d);

    uint8_t extended_family_id = (a >> 20) & 0xFF;
    uint8_t basic_family_id = (a >> 8) & 0xF;
    uint16_t family_id = 0;
    if(basic_family_id == 15) family_id = basic_family_id + extended_family_id;
    else family_id = basic_family_id;

    uint8_t extended_model_id = (a >> 16) & 0xF;
    uint8_t basic_model_id = (a >> 4) & 0xF;
    uint8_t model_id = 0;
    if(basic_family_id == 6 || basic_family_id == 15) model_id = (extended_model_id << 4) | basic_model_id;
    else model_id = basic_model_id;

    uint8_t stepping = a & 0xF;
    uint8_t type = (a >> 12) & 0x3;

    switch (type)
    {
    case 0:
        debug_printf("    Type: Main Processor\n");
        break;
    case 1:
        debug_printf("    Type: Overdrive Processor\n");
        break;
    case 2:
        debug_printf("    Type: MP processor\n");
        break;
    default:
        debug_printf("    Type: Unknown\n");
        break;
    }

    x86_64::cpuid(0, a, b, c, d);
    if(b == signature_INTEL_ebx && c == signature_INTEL_ecx && d == signature_INTEL_edx)
        identify_intel(family_id, model_id, stepping);
    else
        debug_printf("    Unknown vendorID, can't identify further\n"); 
    
    if(x86_64::cpuid(0x80000000, a, b, c, d)){
        if(a >= 0x80000004){
            // Model string supported
            auto print_regs = +[](uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx){
                char str[17] = "";
                *((uint32_t*)str) = eax;
                *((uint32_t*)str + 1) = ebx;
                *((uint32_t*)str + 2) = ecx;
                *((uint32_t*)str + 3) = edx;
                debug_printf("%s", str);
            };
            debug_printf("    Model: ");
            x86_64::cpuid(0x80000002, a, b, c, d);
            print_regs(a, b, c, d);
            x86_64::cpuid(0x80000003, a, b, c, d);
            print_regs(a, b, c, d);
            x86_64::cpuid(0x80000004, a, b, c, d);
            print_regs(a, b, c, d);
            debug_printf("\n");
        }
    }
    

    if(x86_64::cpuid(0x80000000, a, b, c, d)){
        if(a >= 0x80000008){
            x86_64::cpuid(0x80000008, a, b, c, d);
            debug_printf("    Physical address space bits: %d\n", a & 0xFF);
            debug_printf("    Virtual address space bits: %d\n", (a >> 8) & 0xFF);
        }
    }
    
    using namespace cpuid_bits;

    debug_printf("    Features: ");
    if(x86_64::cpuid(1, a, b, c, d)){
        if(d & FPU) debug_printf("fpu ");
        if(d & VME) debug_printf("vme ");
        if(d & DE) debug_printf("de ");
        if(d & PSE) debug_printf("pse ");
        if(d & TSC) debug_printf("tsc ");
        if(d & MSR) debug_printf("msr ");
        if(d & PAE) debug_printf("pae ");
        if(d & MCE) debug_printf("mce ");
        if(d & CX8) debug_printf("cx8 ");
        if(d & APIC) debug_printf("apic ");
        if(d & SEP) debug_printf("sep ");
        if(d & MTRR) debug_printf("mtrr ");
        if(d & PGE) debug_printf("pge ");
        if(d & MCA) debug_printf("mca ");
        if(d & CMOV) debug_printf("cmovcc ");
        if(d & PAT) debug_printf("pat ");
        if(d & PSE36) debug_printf("pse36 ");
        if(d & PSN) debug_printf("psn ");
        if(d & CLFSH) debug_printf("clflush ");
        if(d & DS) debug_printf("ds ");
        if(d & ACPI) debug_printf("acpi ");
        if(d & MMX) debug_printf("mmx ");
        if(d & SSE) debug_printf("sse ");
        if(d & SSE2) debug_printf("sse2 ");
        if(d & SS) debug_printf("ss ");
        if(d & HTT) debug_printf("htt ");
        if(d & TM) debug_printf("tm ");
        if(d & IA64) debug_printf("ia64 ");
        if(d & PBE) debug_printf("#pbe ");
        if(c & SSE3) debug_printf("sse3 ");
        if(c & PCLMUL) debug_printf("pclmul ");
        if(c & DTES64) debug_printf("dtes64 ");
        if(c & MONITOR) debug_printf("monitor ");
        if(c & DSCPL) debug_printf("ds-cpl ");
        if(c & VMX) debug_printf("vmx ");
        if(c & SMX) debug_printf("smx ");
        if(c & EIST) debug_printf("est ");
        if(c & TM2) debug_printf("TM2 ");
        if(c & SSSE3) debug_printf("ssse3 ");
        if(c & CNXTID) debug_printf("cid ");
        if(c & SDBG) debug_printf("sdbg ");
        if(c & FMA) debug_printf("fma ");
        if(c & CMPXCHG16B) debug_printf("cx16 ");
        if(c & xTPR) debug_printf("xtptr ");
        if(c & PDCM) debug_printf("pdcm ");
        if(c & PCID) debug_printf("pcid ");
        if(c & DCA) debug_printf("dca ");
        if(c & SSE4_1) debug_printf("sse4.1 ");
        if(c & SSE4_2) debug_printf("sse4.2 ");
        if(c & x2APIC) debug_printf("x2apic ");
        if(c & MOVBE) debug_printf("movbe ");
        if(c & POPCNT) debug_printf("popcnt ");
        if(c & TSCDeadline) debug_printf("tsc-deadline ");
        if(c & AES) debug_printf("aes ");
        if(c & XSAVE) debug_printf("xsave ");
        if(c & OSXSAVE) debug_printf("osxsave ");
        if(c & AVX) debug_printf("avx ");
        if(c & F16C) debug_printf("f16c ");
        if(c & RDRND) debug_printf("rdrand ");
    }
    
    

    if(x86_64::cpuid(7, a, b, c, d)){
        if(b & FSGSBASE) debug_printf("fsgsbase ");
        if(b & IA32_TSC_ADJUST) debug_printf("IA32_TSC_ADJUST ");
        if(b & SGX) debug_printf("sgx ");
        if(b & BMI1) debug_printf("bmi ");
        if(b & HLE) debug_printf("hle ");
        if(b & AVX2) debug_printf("avx2 ");
        if(b & SMEP) debug_printf("smep ");
        if(b & BMI2) debug_printf("bmi2 ");
        if(b & ENH_MOVSB) debug_printf("enhanced-movsb ");
        if(b & INVPCID) debug_printf("invpcid ");
        if(b & RTM) debug_printf("rtm ");
        if(b & MPX) debug_printf("mpx ");
        if(b & AVX512F) debug_printf("avx-512f ");
        if(b & AVX512DQ) debug_printf("avx-512dq ");
        if(b & RDSEED) debug_printf("rdseed ");
        if(b & ADX) debug_printf("adx ");
        if(b & SMAP) debug_printf("smap ");
        if(b & AVX512IFMA) debug_printf("avx-512ifma ");
        if(b & CLFLUSHOPT) debug_printf("clflushopt ");
        if(b & CLWB) debug_printf("clwb ");
        if(b & AVX512PF) debug_printf("avx-512pf ");
        if(b & AVX512ER) debug_printf("avx-512er ");
        if(b & AVX512CD) debug_printf("avx-512cd ");
        if(b & SHA) debug_printf("sha ");
        if(b & AVX512BW) debug_printf("avx-512bw ");
        if(b & AVX512VL) debug_printf("avx-512vl ");

        if(c & PREFTCHWT1) debug_printf("prefetchwt1 ");
        if(c & AVX512VBMI) debug_printf("avx-512vbmi ");
        if(c & UMIP) debug_printf("umip ");
        if(c & PKU) debug_printf("pku ");
        if(c & OSPKE) debug_printf("ospke ");
        if(c & WAITPKG) debug_printf("waitpkg ");
        if(c & AVX512VBMI2) debug_printf("avx-512vbmi2 ");
        if(c & GFNI) debug_printf("gfni ");
        if(c & VAES) debug_printf("vector-avx ");
        if(c & VPCLMULQDQ) debug_printf("clmul ");
        if(c & AVX512VNNI) debug_printf("avx-512vnni ");
        if(c & AVX512BITALG) debug_printf("avx-512bitalg ");
        if(c & AVX512VPOPCNTDQ) debug_printf("avx-512vpopcntdq ");
        if(c & RDPID) debug_printf("rdpid ");
    }
    debug_printf("\n");
}