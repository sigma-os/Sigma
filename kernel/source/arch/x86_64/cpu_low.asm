[bits 64]
section .text

global emulate_invpcid
emulate_invpcid:
    push rax
    push rdx

    mov rdx, rsi ; Set kernel pml4
    and rdx, ~0xFFF
    or rdx, rdi ; PCID to invalidate
    btc rdx, 63 ; Actually invalidate

    mov rax, cr3

    pushf
    cli ; Don't get interrupted
    mov cr3, rdx ; Invalidate PCID
    mov cr3, rax ; Restore cr3
    popf

    pop rdx
    pop rax
    ret

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