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