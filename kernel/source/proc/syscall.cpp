#include <Sigma/proc/syscall.h>
#include <Sigma/proc/process.h>
//C_LINKAGE void syscall_entry();

static uint64_t syscall_early_klog(uint64_t rbx, uint64_t rcx, uint64_t rdx, uint64_t rsi, uint64_t rdi){
    UNUSED(rcx);
    UNUSED(rdx);
    UNUSED(rsi);
    UNUSED(rdi);
    if(!IS_CANONICAL(rbx)) return 1;
    const char* str = reinterpret_cast<const char*>(rbx);
    debug_printf("[KLOG]: Early: Thread %d says: %s", proc::process::get_current_tid(), str);
    return 0;
}


using syscall_function = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t); // rax, rbx, rcx, rdx, rsi, rdi

struct kernel_syscall {
    syscall_function func;
    const char* name;
};

kernel_syscall syscalls[] = {
    {syscall_early_klog, "early_klog"}
};

constexpr size_t syscall_count = (sizeof(syscalls) / sizeof(kernel_syscall));

static void syscall_handler(x86_64::idt::idt_registers* regs){
    if(regs->rax > syscall_count){
        debug_printf("[SYSCALL]: Tried to access non existing syscall\n");
        regs->rax = 1;
        return;
    }
    
    kernel_syscall& syscall = syscalls[regs->rax];
    #ifdef LOG_SYSCALLS
    debug_printf("[SYSCALL]: Requested syscall %d [%s], from thread %d\n", regs->rax, syscall.name, proc::process::get_current_tid());
    #endif

    regs->rax = syscall.func(regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi);
}


void proc::syscall::init_syscall(){
    //TODO: Implement syscall and sysret instructions
    /*uint32_t eax, ebx, ecx, edx;
    if(x86_64::cpuid(0x80000001, eax, ebx, ecx, edx)) PANIC("Couldn't find cpuid leaf 0x80000001");

    if(bitops<uint32_t>::bit_test(edx, 11)){
        // Syscall and Sysret are supported
        x86_64::msr::write(x86_64::msr::ia32_efer, x86_64::msr::read(x86_64::msr::ia32_efer) | (1ULL << 0)); // Enable SCE (Syscall Extensions)

        x86_64::msr::write(proc::syscall::lstar_msr, reinterpret_cast<uint64_t>(syscall_entry));
        x86_64::msr::write(proc::syscall::sfmask_msr, 0); // Keep all RFLAGS bits as is
    }*/

    

    x86_64::idt::register_interrupt_handler(proc::syscall::syscall_isr_number, syscall_handler, false, true);
}