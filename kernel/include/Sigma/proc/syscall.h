#ifndef SIGMA_KERNEL_PROC_SYSCALL
#define SIGMA_KERNEL_PROC_SYSCALL

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/msr.h>
#include <Sigma/arch/x86_64/idt.h>
#include <Sigma/arch/x86_64/misc/misc.h>


namespace proc::syscall
{
    constexpr uint32_t star_msr = 0xC0000081;
    constexpr uint32_t lstar_msr = 0xC0000082;
    constexpr uint32_t cstar_msr = 0xC0000083;
    constexpr uint32_t sfmask_msr = 0xC0000084;


    constexpr uint8_t syscall_isr_number = 249;

    void init_syscall();

    void set_user_manager_tid(tid_t tid);
} // namespace proc::syscall


#endif