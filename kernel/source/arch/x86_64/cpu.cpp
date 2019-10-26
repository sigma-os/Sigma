#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <klibc/stdio.h>
#include <Sigma/smp/cpu.h>

void x86_64::pat::init(){
    uint32_t a, b, c, d;
    if(cpuid(1, a, b, c, d)){
        if(bitops<uint32_t>::bit_test(d, pat::cpuid_bit)){
            constexpr uint64_t pat = write_back | (write_combining << 8) | (write_through << 16) | (uncacheable << 24);
            x86_64::msr::write(x86_64::msr::ia32_pat, pat);
            debug_printf("[x86_64]: Enabled PAT\n");
        } else {
            PANIC("PAT is not available");
        }
    }
}

void x86_64::umip::init(){
    if(misc::kernel_args::get_bool("noumip")){
        debug_printf("[x86_64]: Forced UMIP disabling\n");
        return;
    }
    uint32_t a, b, c, d;
    if(cpuid(0x7, a, b, c, d)){
        // Nice, the leaf exists
        if(bitops<uint32_t>::bit_test(c, umip::cpuid_bit)){
            // Ladies and gentlemen, we have UMIP, i repeat, we have UMIP
            x86_64::regs::cr4 cr4{};
            cr4.bits.umip = 1; // Enable UMIP
            cr4.flush();
            debug_printf("[x86_64]: Enabled UMIP\n");
        } else {
            debug_printf("[x86_64]: UMIP is not available\n");
        }
    }
}

void x86_64::pcid::init(){
    uint32_t a, b, c, d;
    bool supports_pcid = false, supports_invpcid = false;

    if(!misc::kernel_args::get_bool("nopcid")){
        if(cpuid(1, a, b, c, d))
            if(bitops<uint32_t>::bit_test(c, pcid::pcid_cpuid_bit))
                supports_pcid = true;
    } else
        debug_printf("[x86_64]: Forced pcid disable\n");

    if(!misc::kernel_args::get_bool("noinvpcid")) {
        if(cpuid(0x7, a, b, c, d))
            if(bitops<uint32_t>::bit_test(b, pcid::invpcid_cpuid_bit))
                supports_invpcid = true;
    } else
        debug_printf("[x86_64]: Forced invpcid disable\n");
    
    if(supports_pcid){
        regs::cr4 cr4{};
        cr4.bits.pcide = 1;
        cr4.flush();

        auto* cpu = smp::cpu::entry::get_cpu();
        cpu->features.pcid = supports_pcid;
        cpu->features.invpcid = supports_invpcid;

        debug_printf("[x86_64]: Enabled PCID\n");        
    }
}

void x86_64::misc_features_init(){
    x86_64::umip::init();
    x86_64::pat::init();
    x86_64::pcid::init();
}