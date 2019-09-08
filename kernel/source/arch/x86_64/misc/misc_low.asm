[bits 64]

section .text

global read_xcr
read_xcr:
    push rcx
    push rdx

    xor rax, rax
    xor rdx, rdx

    mov rcx, rdi
    xgetbv

    shl rdx, 32
    or rax, rdx
    
    pop rdx
    pop rcx
    ret

global write_xcr
write_xcr:
    push rax
    push rcx
    push rdx
    push r8

    mov rcx, rdi

    mov r8, rsi
    xor rax, rax 
    mov rax, r8 ; Set low 32bits of 2nd argument into eax

    shr r8, 32
    xor rdx, rdx
    mov rdx, r8 ; Set high 32bits of 2nd argument into eax

    xsetbv

    pop r8
    pop rdx
    pop rcx
    pop rax
    ret