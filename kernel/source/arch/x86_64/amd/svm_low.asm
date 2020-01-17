[bits 64]

section .text

global _vmrun
_vmrun:
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    push rsi

    ; Restore guest GPR state
    mov rax, qword [rdi + 0x0]
    mov rbx, qword [rdi + 0x8]
    mov rcx, qword [rdi + 0x10]
    mov rdx, qword [rdi + 0x18]
    mov rsi, qword [rdi + 0x20]
    mov rbp, qword [rdi + 0x30]
    mov r8, qword [rdi + 0x38]
    mov r9, qword [rdi + 0x40]
    mov r10, qword [rdi + 0x48]
    mov r11, qword [rdi + 0x50]
    mov r12, qword [rdi + 0x58]
    mov r13, qword [rdi + 0x60]
    mov r14, qword [rdi + 0x68]
    mov r15, qword [rdi + 0x70]

    push rdi
    mov rdi, qword [rdi + 0x28]

    mov rax, qword [rsp + 8] ; Move VMCB into rax
    vmload
    vmrun
    vmsave

    push rdi
    mov rdi, qword [rsp + 0x8]  ; Restore RDI from stack

    mov qword [rdi + 0x0], rax
    mov qword [rdi + 0x8], rbx
    mov qword [rdi + 0x10], rcx
    mov qword [rdi + 0x18], rdx
    mov qword [rdi + 0x20], rsi
    mov qword [rdi + 0x30], rbp
    mov qword [rdi + 0x38], r8
    mov qword [rdi + 0x40], r9
    mov qword [rdi + 0x48], r10
    mov qword [rdi + 0x50], r11
    mov qword [rdi + 0x58], r12
    mov qword [rdi + 0x60], r13
    mov qword [rdi + 0x68], r14
    mov qword [rdi + 0x70], r15

    pop r8
    pop r9
    mov qword [r9 + 0x28], r8

    pop rsi
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ret