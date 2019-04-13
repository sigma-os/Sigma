[bits 32]

section .bss

align 16
stack_start:
    resb 0x1000
stack_end:



section .text

extern enter_long_mode

align 4
global _start
_start:
    mov esp, stack_end

    pusha

    call check_multiboot
    call check_cpuid
    call check_long_mode

    call enter_long_mode

    mov dword [0xb8000], 0x2f4b2f4f

    cli
    hlt



check_pae:
    mov eax, 1
    cpuid

    bt edx, 6
    jnc .no_pae

    ret

.no_pae
    mov al, '3'
    call error



check_long_mode:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .no_long_mode
    ret

.no_long_mode:
    mov al, '2'
    ret



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




RAW_VGA_BUFFER equ 0x8b000

MULTIBOOT_MAGIC equ 0x36d76289

x86_FLAGS_ID equ (1 << 21)