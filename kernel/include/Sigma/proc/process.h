#ifndef SIGMA_KERNEL_PROCESS
#define SIGMA_KERNEL_PROCESS

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/drivers/apic.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/acpi/madt.h>
#include <Sigma/smp/cpu.h>
#include <Sigma/types/vector.h>
#include <Sigma/proc/ipc.h>
#include <Sigma/proc/simd.h>

namespace proc::process
{
    struct thread_context {
        thread_context(): rax(0), rbx(0), rcx(0), rdx(0), rsi(0), rdi(0), rbp(0), rsp(0), \
                          r8(0), r9(0), r10(0), r11(0), r12(0), r13(0), r14(0), r15(0), \
                          rip(0), cr3(0), rflags(0), \
                          ss(0), ds(0), cs(0), fs(0), \
                          simd_state(nullptr) {}
        uint64_t rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp; // Normal Registers
        uint64_t r8, r9, r10, r11, r12, r13, r14, r15; // General Purpose Registers
        uint64_t rip, cr3, rflags; // Control Registers
        uint16_t ss, ds, cs; // Segment Registers
        uint64_t fs; // Thread Local Base
        uint8_t* simd_state;

        void copy(thread_context& new_ctx){
            new_ctx.rax = rax;
            new_ctx.rbx = rbx;
            new_ctx.rcx = rcx;
            new_ctx.rdx = rdx;
            new_ctx.rsi = rsi;
            new_ctx.rdi = rdi;

            new_ctx.rbp = rbp;
            new_ctx.rsp = rsp;

            new_ctx.r8 = r8;
            new_ctx.r9 = r9;
            new_ctx.r10 = r10;
            new_ctx.r11 = r11;
            new_ctx.r12 = r12;
            new_ctx.r13 = r13;
            new_ctx.r14 = r14;
            new_ctx.r15 = r15;

            new_ctx.rip = rip;
            new_ctx.cr3 = cr3;
            new_ctx.rflags = rflags;
            new_ctx.ss = ss;
            new_ctx.ds = ds;
            new_ctx.cs = cs;
            new_ctx.fs = fs;

            proc::simd::clone_state(&simd_state, &new_ctx.simd_state);
        }

        void import(x86_64::idt::idt_registers* regs){
            rax = regs->rax;
            rbx = regs->rbx;
            rcx = regs->rcx;
            rdx = regs->rdx;
            rsi = regs->rsi;
            rdi = regs->rdi;
            rbp = regs->rbp;
            rsp = regs->rsp;

            r8 = regs->r8;
            r9 = regs->r9;
            r10 = regs->r10;
            r11 = regs->r11;
            r12 = regs->r12;
            r13 = regs->r13;
            r14 = regs->r14;
            r15 = regs->r15;

            rip = regs->rip;
            rflags = regs->rflags;
            ss = regs->ss;
            ds = regs->ds;
            cs = regs->cs;
        }
    };

    struct thread_resources {
        thread_resources(): frames(types::vector<uint64_t>()) {}
        types::vector<uint64_t> frames;
    };

    constexpr uint64_t mmap_top = 0x7FFF'FFFF'FFFF;
    constexpr uint64_t mmap_bottom = 0x9'0000'0000;
    constexpr uint64_t default_stack_top = 0x800000000;
    constexpr uint64_t default_heap_bottom = 0x400000000;

    struct thread_image {
        thread_image(): stack_top(default_stack_top), stack_bottom(default_stack_top), \
                        heap_bottom(default_heap_bottom), heap_top(default_heap_bottom) {}
        uint64_t stack_top;
        uint64_t stack_bottom;
        uint64_t heap_bottom;
        uint64_t heap_top;
    };

    enum class thread_state {DISABLED, IDLE, RUNNING, BLOCKED};
    enum class block_reason {FOREVER = 0, WAITING_FOR_IPC};
    enum class thread_privilege_level {KERNEL, DRIVER, APPLICATION};

    struct thread {
        thread(): context(proc::process::thread_context()), resources(proc::process::thread_resources()), \
                  image(proc::process::thread_image()), state(proc::process::thread_state::DISABLED), \
                  blocking_reason(block_reason::FOREVER), \
                  privilege(proc::process::thread_privilege_level::APPLICATION), ipc_manager(proc::ipc::thread_ipc_manager()), \
                  vmm(x86_64::paging::paging()), tid(0), thread_lock({}){}
        proc::process::thread_context context;
        proc::process::thread_resources resources;
        proc::process::thread_image image;
        proc::process::thread_state state;
        proc::process::block_reason blocking_reason;
        proc::process::thread_privilege_level privilege;
        proc::ipc::thread_ipc_manager ipc_manager;
        x86_64::paging::paging vmm;
        tid_t tid;
        x86_64::spinlock::mutex thread_lock;
    };
    

    struct managed_cpu {
        managed_cpu() {}
        managed_cpu(smp::cpu_entry cpu, bool enabled, proc::process::thread* current_thread): cpu(cpu), \
             enabled(enabled), current_thread(current_thread) {}
        smp::cpu_entry cpu;
        bool enabled;
        proc::process::thread* current_thread;
    };

    constexpr uint64_t cpu_quantum = 100;
    constexpr uint16_t cpu_quantum_interrupt_vector = 64;

    void init_multitasking(acpi::madt& madt);
    void init_cpu();


    using kernel_thread_function = void (*)();
    proc::process::thread* create_kernel_thread(kernel_thread_function function);
    proc::process::thread* create_thread(void* rip, uint64_t stack, uint64_t cr3, proc::process::thread_privilege_level privilege);
    proc::process::thread* create_blocked_thread(void* rip, uint64_t stack, uint64_t cr3, proc::process::thread_privilege_level privilege);
    void set_thread_state(proc::process::thread* thread, proc::process::thread_state new_state);

    proc::process::thread* thread_for_tid(tid_t tid);
    proc::process::thread* get_current_thread();
    proc::process::managed_cpu* get_current_managed_cpu();
    tid_t get_current_tid();

    void block_thread(tid_t tid, proc::process::block_reason reason, x86_64::idt::idt_registers* regs);
    void block_thread(proc::process::thread* thread, proc::process::block_reason reason, x86_64::idt::idt_registers* regs);

    void wake_thread(tid_t tid);
    void wake_thread(proc::process::thread* thread);

    bool is_blocked(proc::process::thread* thread, proc::process::block_reason reason);
    bool is_blocked(tid_t tid, proc::process::block_reason reason);

    void wake_if_blocked(tid_t tid, proc::process::block_reason reason);
    
    tid_t fork(x86_64::idt::idt_registers* regs);

    // Internal Thread Modifying functions
    void expand_thread_stack(proc::process::thread* thread, size_t pages);
    void* expand_thread_heap(proc::process::thread* thread, size_t pages);

    #define PROT_NONE 0x00
    #define PROT_READ 0x01
    #define PROT_WRITE 0x02
    #define PROT_EXEC 0x04

    #define MAP_PRIVATE 0x01
    #define MAP_SHARED 0x02
    #define MAP_FIXED 0x04
    #define MAP_ANON 0x08

    void* map_anonymous(proc::process::thread* thread, size_t size, void *addr, int prot, int flags);

    // General Thread Modifying functions
    void set_thread_fs(tid_t tid, uint64_t fs);

    // Current Thread Modifying functions
    // You should always use these because they are O(1) and not O(n), where n is the amount of threads, like the general ones
    void set_current_thread_fs(uint64_t fs);
    void kill(x86_64::idt::idt_registers* regs);

    bool receive_message(tid_t* origin, size_t* size, uint8_t* data);
    size_t get_message_size();
} // namespace proc::sched


#endif