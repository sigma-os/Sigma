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

initialize_avx:
    mov eax, 1
    cpuid
    bt ecx, 26
    jnc .no_avx

    mov rax, cr4
    bts rax, 18 ; Set OSXSAVE bit for access to xgetbv and xsetbv
    mov cr4, rax

    mov rcx, 0
    xgetbv
    or eax, 7
    xsetbv

    ret

.no_avx:
    ret

initialize_efer:
    mov ecx, EFER_MSR
    rdmsr
    bts eax, 0 ; Set SCE for the syscall and sysret instructions
    bts eax, 11 ; Set NXE for No-Execute-Support
    wrmsr

    ret

;initialize_cr0:
;    mov rax, cr0
;    bts rax, 16 ; Set Write Protect bit so the CPU will enfore the Writable paging bit in kernel mode
;    mov cr0, rax

;    ret
 
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
    mov rsp, stack_top
    mov rbp, rsp

    call initialize_sse
    call initialize_avx
    call initialize_efer
    ;call initialize_cr0
    call initialize_cr4

    extern _init
    call _init


    cld

    extern kernel_main
    call kernel_main

    extern _fini
    call _fini

    cli

    hlt

global _smp_kernel_early
_smp_kernel_early:
    call initialize_sse
    call initialize_avx
    call initialize_efer
    ;call initialize_cr0
    call initialize_cr4
    
    cld
    extern smp_kernel_main
    call smp_kernel_main

    cli
    hlt


section .bss

align 16
stack_bottom:
    resb 0x4000
stack_top:


RAW_VGA_BUFFER equ (0xb8000 + KERNEL_LMA)

CODE_SEG equ 0x08
DATA_SEG equ 0x10

KERNEL_LMA equ 0xffffffff80000000

EFER_MSR equ 0xC0000080