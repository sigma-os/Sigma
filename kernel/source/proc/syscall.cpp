#include <Sigma/proc/syscall.h>
#include <Sigma/proc/process.h>
#include <Sigma/proc/initrd.h>
#include <Sigma/arch/x86_64/idt.h>

#define SYSCALL_GET_FUNC() (regs->rax)
#define SYSCALL_GET_ARG0() (regs->rbx)
#define SYSCALL_GET_ARG1() (regs->rcx)
#define SYSCALL_GET_ARG2() (regs->rdx)
#define SYSCALL_GET_ARG3() (regs->rsi)
#define SYSCALL_GET_ARG4() (regs->rdi)

#define SYSCALL_SET_RETURN_VALUE(expr) (regs->rax = (expr))

#define PTR_IS_USERLAND(ptr) ((ptr) <= 0x8000000000000000)

#define CHECK_PTR(ptr) if(!IS_CANONICAL((ptr)) || !PTR_IS_USERLAND((ptr)) || (ptr) == 0) return 1

// ARG0: Pointer to str
static uint64_t syscall_early_klog(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    const char* str = reinterpret_cast<const char*>(SYSCALL_GET_ARG0());
    debug_printf("[KLOG]: Early: Thread %d says: %s\n", proc::process::get_current_tid(), str);
    return 0;
}

// ARG0: New FSbase
static uint64_t syscall_set_fsbase(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    proc::process::set_current_thread_fs(SYSCALL_GET_ARG0());
    return 0;
}

// No args
static uint64_t syscall_kill(x86_64::idt::idt_registers* regs){
    proc::process::kill(regs);
    while(1); // Last protection measure
    return 0;
}

// ARG0: Allocate type
//       - 0: SBRK like allocation
//       - 1: Allocation at free base, TODO
// ARG1: Base
// ARG2: n pages

#include <Sigma/mm/alloc.h>
static uint64_t syscall_valloc(x86_64::idt::idt_registers* regs){
    // TODO: Limit amount of allocatable frames
    switch (SYSCALL_GET_ARG0())
    {
    case 0: { // Do sbrk-like allocation
        void* base = proc::process::expand_thread_heap(proc::process::get_current_thread(), SYSCALL_GET_ARG2());
        return reinterpret_cast<uint64_t>(base);
    }
    case 1: {
        PANIC("//TODO");
        break;
    }
    default:
        PANIC("syscall_valloc unknown allocate type");
        break;
    }
    return 0;
}

// ARG0: Char* to filename
// ARG1: uint8_t* to buf
// ARG2: Offset
// ARG3: size

static uint64_t syscall_initrd_read(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    CHECK_PTR(SYSCALL_GET_ARG1());

    if(!proc::initrd::read_file(reinterpret_cast<char*>(SYSCALL_GET_ARG0()), reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG1()), SYSCALL_GET_ARG2(), SYSCALL_GET_ARG3())){
        return 1;
    }

    return 0;
}

// ARG0: void* to addr
// ARG1: size_t length
// ARG2: int prot
// ARG3: int flags

static uint64_t syscall_vm_map(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());

    proc::process::map_anonymous(proc::process::get_current_thread(), SYSCALL_GET_ARG1(), reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG0()), SYSCALL_GET_ARG2(), SYSCALL_GET_ARG3());

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
    {syscall_kill, "kill"},
    {syscall_valloc, "valloc"},
    {syscall_initrd_read, "inird_read"},
    {syscall_vm_map, "vm_map"}
};

constexpr size_t syscall_count = (sizeof(syscalls) / sizeof(kernel_syscall));

static void syscall_handler(x86_64::idt::idt_registers* regs){
    if(SYSCALL_GET_FUNC() > syscall_count){
        debug_printf("[SYSCALL]: Tried to access non existing syscall\n");
        SYSCALL_SET_RETURN_VALUE(1);
        return;
    }
    
    kernel_syscall& syscall = syscalls[SYSCALL_GET_FUNC()];
    #ifdef LOG_SYSCALLS
    debug_printf("[SYSCALL]: Requested syscall %d [%s], from thread %d\n", SYSCALL_GET_FUNC(), syscall.name, proc::process::get_current_tid());
    #endif

    SYSCALL_SET_RETURN_VALUE(syscall.func(regs));
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