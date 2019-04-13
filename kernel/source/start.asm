[bits 32]

section .bss

align 16
boot_stack_bottom:
    resb 0x100
boot_stack_top:



section .text

extern enter_long_mode
extern check_long_mode
extern install_early_gdt

align 4
global _start
_start:
    mov esp, boot_stack_top

    pusha

    call check_multiboot
    call check_cpuid

    call check_pae
    call check_pse

    call check_long_mode
    call enter_long_mode

    call install_early_gdt

    mov al, 'X'
    call error

    cli
    hlt



check_pae:
    mov eax, 1
    cpuid

    bt edx, x86_CPUID_EDX_PAE
    jnc .no_pae

    ret

.no_pae:
    mov al, '2'
    call error

check_pse:
    mov eax, 1
    cpuid

    bt edx, x86_CPUID_EDX_PSE
    jnc .no_pse

    ret

.no_pse:
    mov al, '3'
    call error

check_multiboot:
    cmp eax, MULTIBOOT_MAGIC
    jne .multiboot_magic_failure
    ret

.multiboot_magic_failure:
    mov al, '0'
    call error

check_cpuid:
    pushfd

    pop eax

    mov ecx, eax

    xor eax, x86_FLAGS_ID

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, '1'
    jmp error


global error
error:
    mov dword [RAW_VGA_BUFFER], 0x4f524f45
    mov dword [RAW_VGA_BUFFER + 4], 0x4f3a4f52
    mov dword [RAW_VGA_BUFFER + 8], 0x4f204f20
    mov byte [RAW_VGA_BUFFER + 12], al

    cli
    hlt




RAW_VGA_BUFFER equ 0xb8000

MULTIBOOT_MAGIC equ 0x36d76289

x86_FLAGS_ID equ (1 << 21)

x86_CPUID_EDX_PAE equ 6
x86_CPUID_EDX_PSE equ 3
