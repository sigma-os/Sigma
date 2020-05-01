[bits 64]

section .text

global long_mode_start
long_mode_start:
    mov ax, cs
    cmp ax, CODE_SEG
    jne .no_correct_code_seg

    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    ;mov fs, ax
    ;mov gs, ax


    mov rax, cr3
    add rax, KERNEL_LMA
    mov qword [rax], 0x0
    invlpg [0]

    mov rsp, loader_stack_top
    and rsp, ~8 ; Make sure rsp + 8 is 16 byte aligned as mandated by the SysV ABI
    mov rbp, 0

    extern _start_multiboot_info
    extern _start_multiboot_magic

    mov rsi, 0
    mov esi, dword [_start_multiboot_magic]

    mov rdi, 0
    mov edi, dword [_start_multiboot_info]
    add rdi, KERNEL_LMA

    cld


    [extern kernel_loader_main]
    jmp kernel_loader_main

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

section .bss

align 16
loader_stack_bottom:
    resb 0x4000
loader_stack_top:

section .data
align 0x1000
global p4_table

p4_table:
    dq p3_table - KERNEL_LMA + 3
    times 255 dq 0
    dq p3_table - KERNEL_LMA + 3
    times 254 dq 0
    dq p3_table - KERNEL_LMA + 3

p3_table:
    dq p2_table - KERNEL_LMA + 3
    times 509 dq 0
    dq p2_table - KERNEL_LMA + 3
    times 1 dq 0

%macro gen_pd_2mb 3
	%assign i %1
	%rep %2
		dq (i | 0x83)
		%assign i i+0x200000
	%endrep
	%rep %3
		dq 0
	%endrep
%endmacro

p2_table:
    gen_pd_2mb 0,512,0


KERNEL_LMA equ 0xffffffff80000000
CODE_SEG equ 0x08
RAW_VGA_BUFFER equ (0xb8000 + KERNEL_LMA)