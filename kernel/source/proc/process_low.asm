[bits 64]

section .text

global proc_idle
proc_idle:
    mov rsp, rdi ; Switch stacks

    sti
idle_loop:
    hlt ; Wait for next interrupt

    jmp idle_loop

global xsave_int
xsave_int:
    push rax
    push rdx

    xor rax, rax
    xor rdx, rdx
    mov eax, 0xFFFFFFFF ; Set the Requested Feature BitMap (RFBM) to enable all functions enabled in xcr0
    mov edx, 0xFFFFFFFF

    xsave [rdi]

    pop rdx
    pop rax
    ret

global xrstor_int
xrstor_int:
    push rax
    push rdx

    xor rax, rax
    xor rdx, rdx
    mov eax, 0xFFFFFFFF ; Set the Requested Feature BitMap (RFBM) to enable all functions enabled in xcr0
    mov edx, 0xFFFFFFFF

    xrstor [rdi]

    pop rdx
    pop rdx
    ret
;global syscall_entry
;syscall_entry:
    ; TODO
;   sysret