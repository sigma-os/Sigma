#include <Sigma/proc/simd.h>
#include <Sigma/proc/process.h>
#include <Sigma/arch/x86_64/misc/misc.h>
#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/mm/hmm.h>

C_LINKAGE void xsave_int(uint8_t* state);
C_LINKAGE void xrstor_int(uint8_t* state);

using save_func = void (*)(uint8_t* state);
using restore_func = void (*)(uint8_t* state);

static save_func global_save;
static restore_func global_restore;

static uint8_t* default_state = nullptr;
static uint64_t save_size = 0;
static uint64_t save_align = 0;


static uint8_t* create_state(){
    auto* ptr = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, save_align));
    if(!ptr) return nullptr;
    memcpy(static_cast<void*>(ptr), static_cast<void*>(default_state), save_size);
    return ptr;
}

void proc::simd::save_state(proc::process::thread* thread){
    if(!thread->context.simd_state){
        thread->context.simd_state = create_state();
        if(!thread->context.simd_state) PANIC("Could not allocate space for thread simd state");
    } 

    #ifdef DEBUG
    if(((uint64_t)thread->context.simd_state % save_align) != 0) PANIC("Trying to pass incorrectly aligned pointer to simd save");
    #endif

    global_save(thread->context.simd_state);
}

void proc::simd::restore_state(proc::process::thread* thread){
    if(!thread->context.simd_state){
        thread->context.simd_state = create_state();
        if(!thread->context.simd_state) PANIC("Could not allocate space for thread simd state");
    } 

    #ifdef DEBUG
    if(((uint64_t)thread->context.simd_state % save_align) != 0) PANIC("Trying to pass incorrectly aligned pointer to simd restore");
    #endif

    global_restore(thread->context.simd_state);
}

void proc::simd::clone_state(uint8_t** old_thread, uint8_t** new_thread){
    *new_thread = create_state();

    memcpy(static_cast<void*>(*new_thread), static_cast<void*>(*old_thread), save_size);
}

void proc::simd::init_simd(){
    uint32_t a1, b1, c1, d1;
    if(!x86_64::cpuid(1, a1, b1, c1, d1)) 
        PANIC("Default CPUID leaf does not exist");

    if(c1 & x86_64::cpuid_bits::XSAVE){
        uint32_t a2, b2, c2, d2;
        if(!x86_64::cpuid(0xD, 0, a2, b2, c2, d2)) 
            PANIC("XSAVE exists but CPUID leaf 0xD doesnt exist");
        
        save_size = c2;
        save_align = 64;

        debug_printf("[PROC]: Initializing SIMD saving mechanism with xsave, size: %x\n", save_size);

        global_save = +[](uint8_t* state){
            xsave_int(state);
        };

        global_restore = +[](uint8_t* state){
            xrstor_int(state);
        };
    } else if(d1 & x86_64::cpuid_bits::FXSAVE){
        save_size = 512;
        save_align = 16;
        
        debug_printf("[PROC]: Initializing SIMD saving mechanism with fxsave, size: %x\n", save_size);

        global_save = +[](uint8_t* state){
            asm("fxsave %0" : : "m"(*state));
        };

        global_restore = +[](uint8_t* state){
            asm("fxrstor %0" : : "m"(*state));
        };
    } else {
        PANIC("no known SIMD save mechanism available");
    }

    default_state = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, save_align));
    memset(static_cast<void*>(default_state), 0, save_size);
    auto* tmp = reinterpret_cast<fxsave_area*>(default_state);

    tmp->fcw |= 1 << 0; // Set Invalid Operation Mask
    tmp->fcw |= 1 << 1; // Set Denormal Operand Mask
    tmp->fcw |= 1 << 2; // Set Divide by Zero Mask
    tmp->fcw |= 1 << 3; // Set Overflow Mask
    tmp->fcw |= 1 << 4; // Set Underflow Mask
    tmp->fcw |= 1 << 5; // Set Precision Mask

    tmp->fcw |= (1 << 8) | (1 << 9); // Set Double Extended Precision


    tmp->mxcsr |= 1 << 7; // Set Invalid Operation Mask
	tmp->mxcsr |= 1 << 8; // Set Denormal Mask
	tmp->mxcsr |= 1 << 9; // Set Divide by Zero Mask
	tmp->mxcsr |= 1 << 10; // Set Overflow Mask
	tmp->mxcsr |= 1 << 11; // Set Underflow Mask
	tmp->mxcsr |= 1 << 12; // Set Precision Mask
}