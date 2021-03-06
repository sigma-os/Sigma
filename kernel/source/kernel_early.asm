[bits 64]

section .text

initialize_sse:
    mov eax, 1
    cpuid
    bt edx, 25
    jnc .no_sse

    mov rax, cr0
    btr eax, 2
    bts eax, 1
    mov cr0, rax

    mov rax, cr4
    bts eax, 9
    bts eax, 10
    mov cr4, rax

    ret

.no_sse:    
    ret

initialize_osxsave:
    mov eax, 1
    cpuid
    bt ecx, 26
    jnc .no_osxsave

    mov rax, cr4
    bts rax, 18 ; Set OSXSAVE bit for access to xgetbv and xsetbv and possibly xsave, xsaveopt and xrestor
    mov cr4, rax

    ret

.no_osxsave:
    ret

initialize_efer:
    mov ecx, 0xC0000080 ; IA32_EFER
    rdmsr
    bts eax, 0 ; Set SCE for the syscall and sysret instructions
    bts eax, 11 ; Set NXE for No-Execute-Support
               ; LME and thus LMA are already set by the bootloader
    wrmsr

    ret

initialize_cr0:
    mov rax, cr0
    bts rax, 16 ; Set Write Protect bit so the CPU will enfore the Writable paging bit in kernel mode
    mov cr0, rax

    ret

initialize_cr4:
    xor rax, rax
    mov eax, 1
    cpuid
    bt edx, 13
    jnc .no_pge

    mov rax, cr4
    bts rax, 7 ; Set Page Global Enable
    mov cr4, rax

.no_pge:
    ret

global _kernel_early
_kernel_early:
    cld
    cli

    mov rsp, bsp_stack_top
    and rsp, ~8 ; Make sure rsp + 8 is 16 byte aligned as mandated by the SysV ABI
    mov rbp, 0 ; Set to zero to provide stack trace stop

    call initialize_sse
    call initialize_osxsave
    call initialize_efer
    call initialize_cr0
    call initialize_cr4

    extern _init
    call _init
    
    extern kernel_main
    call kernel_main

    extern _fini
    call _fini

    cli

    hlt

global _smp_kernel_early
_smp_kernel_early:
    cli
    cld

    mov rax, qword [trampoline_paging]
    mov cr3, rax

    mov rsp, qword [trampoline_stack]
    and rsp, ~0xF ; Align stack for ABI requirements
    mov rbp, 0 ; Set to zero to provide stack trace stop
    
    mov byte [trampoline_booted], 1

    call initialize_sse
    call initialize_osxsave
    call initialize_efer
    call initialize_cr0
    call initialize_cr4
    
    extern smp_kernel_main
    call smp_kernel_main

    cli
    hlt


section .data

global trampoline_stack
trampoline_stack: dq 0
global trampoline_paging
trampoline_paging: dq 0
global trampoline_booted
trampoline_booted: db 0

section .bss

align 16
bsp_stack_bottom:
    resb 0x8000
bsp_stack_top: