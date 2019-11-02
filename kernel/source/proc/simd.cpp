#include <Sigma/proc/simd.h>
#include <Sigma/proc/process.h>
#include <Sigma/arch/x86_64/misc/misc.h>
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
    if(((uint64_t)thread->context.simd_state % save_align) != 0) PANIC("Trying to pass uncorrectly aligned pointer to simd save");
    #endif

    global_save(thread->context.simd_state);
}

void proc::simd::restore_state(proc::process::thread* thread){
    if(!thread->context.simd_state){
        thread->context.simd_state = create_state();
        if(!thread->context.simd_state) PANIC("Could not allocate space for thread simd state");
    } 

    #ifdef DEBUG
    if(((uint64_t)thread->context.simd_state % save_align) != 0) PANIC("Trying to pass uncorrectly aligned pointer to simd restore");
    #endif

    global_restore(thread->context.simd_state);
}

void proc::simd::clone_state(uint8_t** old_thread, uint8_t** new_thread){
    *new_thread = create_state();

    memcpy(static_cast<void*>(*new_thread), static_cast<void*>(*old_thread), save_size);
}

void proc::simd::init_simd(){
    uint32_t a1, b1, c1, d1;
    if(!x86_64::cpuid(1, a1, b1, c1, d1)) PANIC("Default CPUID leaf does not exist");

    if(c1 & bit_XSAVE){
        uint32_t a2, b2, c2, d2;
        if(!x86_64::cpuid(0xD, 0, a2, b2, c2, d2)) PANIC("XSAVE exists but CPUID leaf 0xD doesnt exist");
        
        save_size = c2;
        save_align = 64;

        debug_printf("[PROC]: Initializing saving mechanism with xsave, size: %x\n", save_size);

        global_save = +[](uint8_t* state){
            xsave_int(state);
        };

        global_restore = +[](uint8_t* state){
            xrstor_int(state);
        };
    } else if(d1 & bit_FXSAVE){
        save_size = 512;
        save_align = 16;
        
        debug_printf("[PROC]: Initializing saving mechanism with fxsave, size: %x\n", save_size);

        global_save = +[](uint8_t* state){
            asm("fxsave (%0)" : : "r"(state) : "memory");
        };

        global_restore = +[](uint8_t* state){
            asm("fxrstor (%0)" : : "r"(state) : "memory");
        };
    } else PANIC("no known SIMD save mechanism available");

    default_state = static_cast<uint8_t*>(mm::hmm::kmalloc_a(save_size, save_align));
    memset(static_cast<void*>(default_state), 0, save_size);
    global_save(default_state);
}

// TODO: This is ugly, move this somewhere better or make it clearer
void proc::simd::init_ap_simd(){
    uint32_t a1, b1, c1, d1;
    if(!x86_64::cpuid(1, a1, b1, c1, d1)) PANIC("Default CPUID leaf does not exist");

    if(c1 & bit_XSAVE){
        // TODO, move this to an x86_64::regs::xcr0 class
        uint64_t xcr0 = 0;
        bitops<uint64_t>::bit_set(xcr0, 0); // Set FPU bit, since it is *always* required to be set
        if(d1 & bit_SSE) bitops<uint64_t>::bit_set(xcr0, 1); // Enable SSE, is always done becuase this is long mode but check because why not
        if(c1 & bit_AVX) bitops<uint64_t>::bit_set(xcr0, 2); // Enable AVX-256

        x86_64::write_xcr(0, xcr0);
    }
}