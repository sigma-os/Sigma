#include <Sigma/proc/syscall.h>
#include <Sigma/proc/process.h>
#include <Sigma/proc/initrd.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/cpu.h>
#include <Sigma/proc/device.h>
#include <atomic>

#define SYSCALL_GET_FUNC() (regs->rax)
#define SYSCALL_GET_ARG0() (regs->rbx)
#define SYSCALL_GET_ARG1() (regs->rcx)
#define SYSCALL_GET_ARG2() (regs->rdx)
#define SYSCALL_GET_ARG3() (regs->rsi)
#define SYSCALL_GET_ARG4() (regs->rdi)

#define SYSCALL_SET_RETURN_VALUE(expr) (regs->rax = (expr))

#define PTR_IS_USERLAND(ptr) ((ptr) <= 0x8000000000000000)

#define CHECK_PTR(ptr) \
	if(!misc::is_canonical((ptr)) || !PTR_IS_USERLAND((ptr)) || (ptr) == 0){ \
        debug_printf("[SYSCALL]: Pointer check failed [%x]\n", ptr); \
	    return 1; \
    }

// ARG0: Pointer to str
static uint64_t syscall_early_klog(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    const char* str = reinterpret_cast<const char*>(SYSCALL_GET_ARG0());
    debug_printf("%s", str);
    return 0;
}

// ARG0: New FSbase
static uint64_t syscall_set_fsbase(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    proc::process::get_current_thread()->set_fsbase(SYSCALL_GET_ARG0());
    return 0;
}

// No args
static uint64_t syscall_kill(x86_64::idt::idt_registers* regs){
    proc::process::kill(regs);
    while(1); // Last protection measure
    return 0;
}

// ARG0: void* to virtual addr
// ARG1: void* to physical addr, only allowed if thread is DRIVER level or KERNEL
// ARG2: size_t length
// ARG3: int prot
// ARG4: int flags
// RET: addr
static uint64_t syscall_vm_map(x86_64::idt::idt_registers* regs){
    if(!PTR_IS_USERLAND(SYSCALL_GET_ARG0()))
        return 1;

    if(SYSCALL_GET_ARG1() != 0 && proc::process::get_current_thread()->privilege < proc::process::thread_privilege_level::DRIVER)
        return 1;

    
    auto ret = proc::process::get_current_thread()->map_anonymous(SYSCALL_GET_ARG2(), reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG0()), reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG1()), SYSCALL_GET_ARG3(), SYSCALL_GET_ARG4());

    CHECK_PTR((uint64_t)ret);

    return reinterpret_cast<uint64_t>(ret);
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

// ARG0: Char* to filename
// RET: size
static uint64_t syscall_initrd_get_size(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());

    return proc::initrd::get_size(reinterpret_cast<char*>(SYSCALL_GET_ARG0()));
}

// ARG0: destination
// ARG1: buf size
// ARG2: buf
static uint64_t syscall_send_message(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG2());

    proc::process::thread* thread = proc::process::thread_for_tid(SYSCALL_GET_ARG0());
    if(!thread->ipc_manager.send_message(proc::process::get_current_tid(), SYSCALL_GET_ARG1(), reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG2()))){
        return 1;
    }

    return 0;
}

// ARG0: buf
// ARG1: pointer to size_t (buf_size)
// ARG2: pointer to tid (origin)
static uint64_t syscall_receive_message(x86_64::idt::idt_registers* regs){
    CHECK_PTR(SYSCALL_GET_ARG0());
    CHECK_PTR(SYSCALL_GET_ARG1());
    CHECK_PTR(SYSCALL_GET_ARG2());

    uint8_t* buf_ptr = reinterpret_cast<uint8_t*>(SYSCALL_GET_ARG0());
    uint64_t* buf_size_ptr = reinterpret_cast<uint64_t*>(SYSCALL_GET_ARG1());
    tid_t* origin_ptr = reinterpret_cast<tid_t*>(SYSCALL_GET_ARG2());
    if(!proc::process::receive_message(origin_ptr, buf_size_ptr, buf_ptr)){
        return 1;
    }
    return 0;
}

static uint64_t syscall_get_message_size(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs) {
	return proc::process::get_message_size();
}

static std::atomic<tid_t> um_tid = 0;
static uint64_t syscall_get_um_tid(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs) {
	return um_tid;
}

// ARG0: Reason
static uint64_t syscall_block_thread(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs){
    proc::process::get_current_thread()->block(static_cast<proc::process::block_reason>(SYSCALL_GET_ARG0()), regs);
    return 0;
}

static uint64_t syscall_fork(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs){
    return proc::process::fork(regs);
}

// ARG0: Command
// ARG1 - N: Optional args to devctl
static uint64_t syscall_devctl(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs){
    return proc::device::devctl(SYSCALL_GET_ARG0(), SYSCALL_GET_ARG1(), SYSCALL_GET_ARG2(), SYSCALL_GET_ARG3(), SYSCALL_GET_ARG4());
}

static uint64_t syscall_get_current_tid(MAYBE_UNUSED_ATTRIBUTE x86_64::idt::idt_registers* regs){
    return proc::process::get_current_tid();
}

// ARG0: Size in bytes
// ARG1: prot
// ARG2: flags
// ARG3: pointer to phys_region
static uint64_t syscall_get_phys_region(x86_64::idt::idt_registers* regs){
    if(!PTR_IS_USERLAND(SYSCALL_GET_ARG3()))
        return 1;

    if(proc::process::get_current_thread()->privilege < proc::process::thread_privilege_level::DRIVER)
        return 1;

    
    auto ret = proc::process::get_current_thread()->get_phys_region(SYSCALL_GET_ARG0(), SYSCALL_GET_ARG1(), SYSCALL_GET_ARG2(), (proc::process::thread::phys_region*)SYSCALL_GET_ARG3());
        
    if(ret == false)
        return 1;

    return 0; // Return success
}

// ARG0: command
// ARG1 - N: argn
static uint64_t syscall_vctl(x86_64::idt::idt_registers* regs){
    return virt::vctl(SYSCALL_GET_ARG0(), SYSCALL_GET_ARG1(), SYSCALL_GET_ARG2(), SYSCALL_GET_ARG3(), SYSCALL_GET_ARG4());
}



using syscall_function = uint64_t (*)(x86_64::idt::idt_registers*);

struct kernel_syscall {
    syscall_function func;
    const char* name;
};

kernel_syscall syscalls[] = {
    {.func = syscall_early_klog, .name = "early_klog"},

    {.func = syscall_set_fsbase, .name = "set_fsbase"},
    {.func = syscall_kill, .name = "kill"},
    {.func = syscall_fork, .name = "fork"},
    {.func = syscall_get_current_tid, .name = "get_current_tid"},
    {.func = syscall_get_um_tid, .name = "get_user_manager_tid"},
    {.func = syscall_block_thread, .name = "block_thread"},

    {.func = syscall_vm_map, .name = "vm_map"},
    {.func = syscall_get_phys_region, .name = "get_phys_region"},

    {.func = syscall_initrd_read, .name = "initrd_read"},
    {.func = syscall_initrd_get_size, .name = "initrd_get_size"},

    {.func = syscall_send_message, .name = "ipc_send"},
    {.func = syscall_receive_message, .name = "ipc_receive"},
    {.func = syscall_get_message_size, .name = "ipc_get_message_size"},

    {.func = syscall_devctl, .name = "devctl"},
    {.func = syscall_vctl, .name = "vctl"},
};

constexpr size_t syscall_count = (sizeof(syscalls) / sizeof(kernel_syscall));

static void syscall_handler(x86_64::idt::idt_registers* regs){
    if(SYSCALL_GET_FUNC() >= syscall_count){
        debug_printf("[SYSCALL]: Tried to access non existing syscall\n");
        SYSCALL_SET_RETURN_VALUE(1);
        return;
    }

    uint64_t func = SYSCALL_GET_FUNC();
    
    kernel_syscall& syscall = syscalls[func];

    {
        x86_64::smap::smap_guard guard{};
        SYSCALL_SET_RETURN_VALUE(syscall.func(regs));
    }

    #ifdef LOG_SYSCALLS
    debug_printf("[SYSCALL]: Requested syscall %d [%s(%x, %x, %x, %x, %x)], from thread %d -> return: %x\n", func, syscall.name, SYSCALL_GET_ARG0(), SYSCALL_GET_ARG1(), SYSCALL_GET_ARG2(), SYSCALL_GET_ARG3(), SYSCALL_GET_ARG4(), proc::process::get_current_tid(), SYSCALL_GET_FUNC());
    #endif
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

void proc::syscall::set_user_manager_tid(tid_t tid){
    um_tid = tid;
}