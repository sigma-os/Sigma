#include <Sigma/proc/syscall.h>
#include <Sigma/proc/process.h>

#include <Sigma/arch/x86_64/idt.h>

static uint64_t syscall_early_klog(x86_64::idt::idt_registers* regs){
    if(!IS_CANONICAL(regs->rbx)) return 1;
    const char* str = reinterpret_cast<const char*>(regs->rbx);
    debug_printf("[KLOG]: Early: Thread %d says: %s", proc::process::get_current_tid(), str);
    return 0;
}

static uint64_t syscall_set_fsbase(x86_64::idt::idt_registers* regs){
    proc::process::set_current_thread_fs(regs->rbx);
    return 0;
}

static uint64_t syscall_kill(x86_64::idt::idt_registers* regs){
    proc::process::kill(regs);
    while(1); // Last protection measure
    return 0;
}

using syscall_function = uint64_t (*)(x86_64::idt::idt_registers*);

struct kernel_syscall {
    syscall_function func;
    const char* name;
};

kernel_syscall syscalls[] = {
    {syscall_early_klog, "early_klog"},
    {syscall_set_fsbase, "set_fsbase"},
    {syscall_kill, "kill"}
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

    regs->rax = syscall.func(regs);
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