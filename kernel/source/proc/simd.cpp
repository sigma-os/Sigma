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

void proc::simd::simd_state::init(){
    if(!data){
        data = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, save_align));
        if(!data)
            PANIC("Couldn't allocate data for simd_state");

        #ifdef DEBUG
        if(((uint64_t)data % save_align) != 0)
            PANIC("mm::hmm::kmalloc_a didn't allocate sufficiently aligned pointer");
        #endif
    }

    memcpy(data, default_state, save_size);
}

void proc::simd::simd_state::deinit(){
    if(data) {
        mm::hmm::kfree(data);
        data = nullptr;
    }
}

void proc::simd::simd_state::save(){
    if(!data)
        init();
    
    global_save(data);
}

void proc::simd::simd_state::restore(){
    if(!data)
        init();
    
    global_restore(data);
}

proc::simd::simd_state& proc::simd::simd_state::operator=(const simd_state& state){
    init();

    memcpy(data, state.data, save_size);

    return *this;
}

void proc::simd::init(){
    uint32_t a1, b1, c1, d1;
    if(!x86_64::cpuid(1, a1, b1, c1, d1)) 
        PANIC("Default CPUID leaf does not exist");

    if(c1 & x86_64::cpuid_bits::XSAVE){
        uint32_t a2, b2, c2, d2;
        if(!x86_64::cpuid(0xD, 0, a2, b2, c2, d2)) 
            PANIC("XSAVE exists but CPUID leaf 0xD doesnt exist");
        
        save_size = c2;
        save_align = 64;

        global_save = xsave_int;
        global_restore = xrstor_int;

        debug_printf("[PROC]: Initializing SIMD saving mechanism with xsave, size: %x\n", save_size);
    } else if(d1 & x86_64::cpuid_bits::FXSAVE){
        save_size = 512;
        save_align = 16;

        global_save = +[](uint8_t* state){ asm("fxsave %0" : : "m"(*state)); };
        global_restore = +[](uint8_t* state){ asm("fxrstor %0" : : "m"(*state)); };
        
        debug_printf("[PROC]: Initializing SIMD saving mechanism with fxsave, size: %x\n", save_size);
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