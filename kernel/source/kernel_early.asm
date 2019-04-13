bits 64

section .text

global long_mode_start
long_mode_start:
    mov ax, cs
    cmp ax, CODE_SEG
    jne .no_correct_code_seg

    mov ax, DATA_SEG
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    jmp _kernel_early

.no_correct_code_seg:
    mov al, '6'
    call long_mode_error


long_mode_error:
    mov dword [RAW_VGA_BUFFER], 0x4f524f45
    mov dword [RAW_VGA_BUFFER + 4], 0x4f3a4f52
    mov dword [RAW_VGA_BUFFER + 8], 0x4f204f20
    mov byte [RAW_VGA_BUFFER + 12], al

    cli
    hlt

_kernel_early:
    mov esp, stack_top
    mov ebp, esp

    mov rax, 0x2f592f412f4b2f4f
    mov qword [RAW_VGA_BUFFER], rax

    cli

    hlt


section .bss

align 16
stack_bottom:
    resb 0x1000
stack_top:


RAW_VGA_BUFFER equ 0xb8000

CODE_SEG equ 0x08
DATA_SEG equ 0x10