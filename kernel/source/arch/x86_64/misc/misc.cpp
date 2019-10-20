#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/smp/cpu.h>
#include <klibcxx/mutex.hpp>
#include <klibc/stdio.h>

bool x86_64::cpuid(uint32_t leaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx){
    return __get_cpuid(leaf, &eax, &ebx, &ecx, &edx);
}

bool x86_64::cpuid(uint32_t leaf, uint32_t subleaf, uint32_t& eax, uint32_t& ebx, uint32_t& ecx, uint32_t& edx){
    return __get_cpuid_count(leaf, subleaf, &eax, &ebx, &ecx, &edx);
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
            debug_printf("    Model: P3 (0.18 micrometer) with 2 MB on-die L2");
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
    
    x86_64::cpuid(0x80000000, a, b, c, d);
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