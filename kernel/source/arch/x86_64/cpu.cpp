#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <klibc/stdio.h>

#include <Sigma/arch/x86_64/amd/svm.hpp>

void x86_64::smep::init(){
    if(misc::kernel_args::get_bool("nosmep")){
        debug_printf("[CPU]: Forced SMEP disabling\n");
        return;
    }
    uint32_t a, b, c, d;
    if(cpuid(0x7, 0, a, b, c, d)){
        // Nice, the leaf exists
        if(b & cpuid_bits::SMEP){
            x86_64::regs::cr4 cr4{};
            cr4.bits.smep = 1; // Enable SMEP
            cr4.flush();
            debug_printf("[CPU]: Enabled SMEP\n");
        } else {
            debug_printf("[CPU]: SMEP is not available\n");
        }
    }
}

void x86_64::smap::init(){
    if(misc::kernel_args::get_bool("nosmap")){
        debug_printf("[CPU]: Forced SMAP disabling\n");
        return;
    }
    uint32_t a, b, c, d;
    if(cpuid(0x7, 0, a, b, c, d)){
        // Nice, the leaf exists
        if(b & cpuid_bits::SMAP){
            x86_64::regs::cr4 cr4{};
            cr4.bits.smap = 1; // Enable SMAP
            cr4.flush();

            smp::cpu::entry::get_cpu()->features.smap = 1;

            asm("clac");
            debug_printf("[CPU]: Enabled SMAP\n");
        } else {
            debug_printf("[CPU]: SMAP is not available\n");
        }
    }
}

void x86_64::pat::init(){
    uint32_t a, b, c, d;
    if(cpuid(1, a, b, c, d)){
        if(d & cpuid_bits::PAT){
            x86_64::msr::write(x86_64::msr::ia32_pat, pat::sigma_pat);
            debug_printf("[CPU]: Enabled PAT\n");
        } else {
            PANIC("PAT is not available");
        }
    }
}

void x86_64::umip::init(){
    if(misc::kernel_args::get_bool("noumip")){
        debug_printf("[CPU]: Forced UMIP disabling\n");
        return;
    }
    uint32_t a, b, c, d;
    if(cpuid(0x7, 0, a, b, c, d)){
        // Nice, the leaf exists
        if(c & cpuid_bits::UMIP){
            // Ladies and gentlemen, we have UMIP, i repeat, we have UMIP
            x86_64::regs::cr4 cr4{};
            cr4.bits.umip = 1; // Enable UMIP
            cr4.flush();
            debug_printf("[CPU]: Enabled UMIP\n");
        } else {
            debug_printf("[CPU]: UMIP is not available\n");
        }
    }
}

void x86_64::pcid::init(){
    uint32_t a, b, c, d;
    bool supports_pcid = false, supports_invpcid = false;

    if(!misc::kernel_args::get_bool("nopcid")){
        if(cpuid(1, a, b, c, d))
            if(c & cpuid_bits::PCID)
                supports_pcid = true;
    } else
        debug_printf("[CPU]: Forced pcid disable\n");

    if(!misc::kernel_args::get_bool("noinvpcid")) {
        if(cpuid(0x7, 0, a, b, c, d))
            if(b & cpuid_bits::INVPCID)
                supports_invpcid = true;
                
    } else
        debug_printf("[CPU]: Forced invpcid disable\n");
    
    if(supports_pcid){
        regs::cr4 cr4{};
        cr4.bits.pcide = 1;
        cr4.flush();

        auto* cpu = smp::cpu::entry::get_cpu();
        cpu->features.pcid = supports_pcid;
        cpu->features.invpcid = supports_invpcid;

        debug_printf("[CPU]: Enabled PCID\n");        
    } else {
        debug_printf("[CPU]: PCID is not available\n");
    }
}

void x86_64::tsd::init(){
    // Assume the TSC is supported since it is *way* older than x86_64
    if(misc::kernel_args::get_bool("enable_tsd")){
        regs::cr4 cr4{};
        cr4.bits.tsd = 1;
        cr4.flush();
        
        debug_printf("[CPU]: Enabled TSD\n");
    }
}

// Intel paper on TME: https://web.archive.org/web/20190514134553/http://kib.kiev.ua/x86docs/SDMs/336907-001.pdf
void x86_64::tme::init(){
    if(!misc::kernel_args::get_bool("notme")){
        uint32_t a, b, c, d;
        if(cpuid(7, 0, a, b, c, d)){
            if(c & cpuid_bits::TME){
                // TME exists, woooo

                uint32_t activate = x86_64::msr::read(x86_64::msr::ia32_tme_activate);
                if(bitops<uint32_t>::bit_test(activate, 0)){
                    debug_printf("[CPU]: TME was already enabled by BIOS or FW\n");
                } else {
                    uint32_t capability = x86_64::msr::read(x86_64::msr::ia32_tme_capability);
                    if(bitops<uint32_t>::bit_test(capability, 0)){
                        // AES-XTS 128bit encryption supported, try to enable
                        
                        // Enable, Generate new key, Save key to storage for use when out of standby, AES-XTS 128bit encryption
                        x86_64::msr::write(x86_64::msr::ia32_tme_activate, (1 << 1) | (1 << 3));
                        printf("WARNING: TME support is fully untested, use at your own risk, pass `notme` to the kernel to disable it\n");
                        debug_printf("[CPU]: Enabled TME\n");
                    } else {
                        debug_printf("[CPU]: No available TME encryption algorithm\n");
                    }
                }
            } else {
                debug_printf("[CPU]: TME is not available\n");
            }
        }
    }
}

void x86_64::tme::restore_key(){
    if(!misc::kernel_args::get_bool("notme")){
        uint32_t a, b, c, d;
        if(cpuid(7, 0, a, b, c, d)){
            if(c & cpuid_bits::TME){
                // TME exists, woooo
                uint32_t activate = x86_64::msr::read(x86_64::msr::ia32_tme_activate);
                if(bitops<uint32_t>::bit_test(activate, 0)){
                    debug_printf("[CPU]: TME was already enabled by BIOS or FW\n");
                } else {
                    uint32_t capability = x86_64::msr::read(x86_64::msr::ia32_tme_capability);
                    if(bitops<uint32_t>::bit_test(capability, 0)){
                        // AES-XTS 128bit encryption supported, try to enable
                        
                        // Enable, Retrieve key from storage, AES-XTS 128bit encryption
                        x86_64::msr::write(x86_64::msr::ia32_tme_activate, (1 << 1) | (1 << 2));
                        debug_printf("[CPU]: Re-enabled TME with stored key\n");
                    } else {
                        debug_printf("[CPU]: No available TME encryption algorithm");
                    }
                }
            } else {
                debug_printf("[CPU]: TME is not available\n");
            }
        }
    }
}

void x86_64::misc_bsp_late_features_init(){
    x86_64::tme::init();
}

void x86_64::misc_early_features_init(){
    x86_64::tsd::init();
    x86_64::smep::init();
    x86_64::smap::init();
    x86_64::umip::init();
    x86_64::pat::init();
    x86_64::pcid::init();

    uint32_t a, b, c, d;
    x86_64::cpuid(0, a, b, c, d);
    if(b == signature_AMD_ebx && c == signature_AMD_ecx && d == signature_AMD_edx){
        x86_64::svm::init();
    }

    
    x86_64::cpuid(1, a, b, c, d);
    if(c & cpuid_bits::XSAVE){
        // xcr0 specifies what the `xsave` instruction should save, however it is also used for enabling instruction sets, e.g. AVX
        x86_64::regs::xcr0 xcr{};
        xcr.bits.fpu_mmx = 1; // Set FPU bit, since it is *always* required to be set
        if(d & cpuid_bits::SSE) xcr.bits.sse = 1; // Enable SSE, is always done becuase this is long mode but check because why not
        if(c & cpuid_bits::AVX) xcr.bits.avx = 1; // Enable AVX
        xcr.flush();
    }
}

#pragma region CPU Model Identification

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

static void identify_amd(uint16_t family_id, uint8_t model_id, uint8_t stepping){
    auto identify_family_23 = [&](){
        debug_printf("    Family: AMD Zen\n");
        switch (model_id)
        {
        case 1:
            debug_printf("    Model: Naples, Whitehaven, Summit Ridge, Snowy Owl\n");
            break;
        case 0x11:
            debug_printf("    Model: Raven Ridge\n");
            break;
        case 0x8:
            debug_printf("    Model: Pinnacle Ridge\n");
            break;
        case 0x18:
            debug_printf("    Model: Picasso\n");
            break;
        case 0x71:
            debug_printf("    Model: Matisse\n");
            break;
        default:
            debug_printf("    Model: Unknown [%x]\n", model_id);
            break;
        }

        debug_printf("    Stepping: %x\n", stepping);
    };


    switch (family_id)
    {
    case 23:
        identify_family_23();
        break;
    default:
        debug_printf("    Family: Unknown [%x]\n", family_id);
        break;
    }
}

void x86_64::identify_cpu(){
    debug_printf("[CPU]: Detecting CPU with id: %d\n", smp::cpu::get_current_cpu()->lapic_id);

    uint32_t a = 0, b = 0, c = 0, d = 0;
    x86_64::cpuid(0, a, b, c, d);

    uint32_t vendorID[5] = {};
    *(vendorID) = b;
    *(vendorID + 1) = d;
    *(vendorID + 2) = c;

    debug_printf("    VendorID: %s\n", (const char*)vendorID);

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
    else if(b == signature_AMD_ebx && c == signature_AMD_ecx && d == signature_AMD_edx)
        identify_amd(family_id, model_id, stepping);
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
        if(d & FXSAVE) debug_printf("fxsave ");
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
    
    if(x86_64::cpuid(7, 0, a, b, c, d)){
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
        if(c & TME) debug_printf("TME ");
        if(c & AVX512BITALG) debug_printf("avx-512bitalg ");
        if(c & AVX512VPOPCNTDQ) debug_printf("avx-512vpopcntdq ");
        if(c & RDPID) debug_printf("rdpid ");
    }

    if(x86_64::cpuid(0x80000001, a, b, c, d)){
        if(d & SYSCALL) debug_printf("syscall ");
        if(d & NX) debug_printf("nx ");
        if(d & FFXSR) debug_printf("fast-fxsave ");
        if(d & PDPE1GB) debug_printf("pdpe1gb ");
        if(d & RDTSCP) debug_printf("rdtscp ");
        if(d & LM) debug_printf("lm ");
        
        if(c & SVM) debug_printf("svm ");
    }
    debug_printf("\n");
}

#pragma endregion
