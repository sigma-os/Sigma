#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <klibc/stdio.h>

void x86_64::pat::init(){
    uint32_t a, b, c, d;
    if(cpuid(1, a, b, c, d)){
        if(bitops<uint32_t>::bit_test(d, 16)){
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
        }
    }
}

void x86_64::misc_features_init(){
    x86_64::umip::init();
    x86_64::pat::init();
}