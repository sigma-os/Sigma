#include <Sigma/proc/simd.h>
#include <Sigma/proc/process.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/mm/hmm.h>

uint8_t* default_state = nullptr;
uint64_t save_size = 0;
bool use_xsave = false;

static uint8_t* create_state(){
    auto* ptr = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, 16));
    if(!ptr) return nullptr;
    memcpy(static_cast<void*>(ptr), static_cast<void*>(default_state), save_size);
    return ptr;
}

static void save_state_int(uint8_t* state){
    if(use_xsave) asm("xsave (%0)" : : "r"(state) : "memory");
    else asm("fxsave (%0)" : : "r"(state) : "memory");
}

static void restore_state_int(uint8_t* state){
    if(use_xsave) asm("xrstor (%0)" : : "r"(state) : "memory");
    else asm("fxrstor (%0)" : : "r"(state) : "memory");
}

void proc::simd::save_state(proc::process::thread* thread){
    if(!thread->context.simd_state){
        thread->context.simd_state = create_state();
        if(!thread->context.simd_state) PANIC("Could not allocate space for thread simd state");
    } 
    save_state_int(thread->context.simd_state);
}

void proc::simd::restore_state(proc::process::thread* thread){
    if(!thread->context.simd_state){
        thread->context.simd_state = create_state();
        if(!thread->context.simd_state) PANIC("Could not allocate space for thread simd state");
    } 
    restore_state_int(thread->context.simd_state);
}



void proc::simd::init_simd(){
    uint32_t a1, b1, c1, d1;
    if(!x86_64::cpuid(1, a1, b1, c1, d1)) PANIC("Default CPUID leaf does not exist");

    if(c1 & bit_XSAVE){
        uint32_t a2, b2, c2, d2;
        if(!x86_64::cpuid(0xD, a2, b2, c2, d2)) PANIC("XSAVE exists but CPUID leaf 0xD doesnt exist");

        save_size = b2;
        use_xsave = true;
        debug_printf("[PROC]: Initializing saving mechanism with xsave, size: %x\n", save_size);
    } else if(d1 & bit_FXSAVE){
        save_size = 512;
        use_xsave = false;
        debug_printf("[PROC]: Initializing saving mechanism with fxsave, size: %x\n", save_size);
    } else PANIC("no known SIMD save mechanism available");

    default_state = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, 16));
    memset(static_cast<void*>(default_state), 0, save_size);
    save_state_int(default_state);
}