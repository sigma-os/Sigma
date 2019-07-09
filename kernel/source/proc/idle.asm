[bits 64]

section .text

global proc_idle
proc_idle:
    mov rsp, rdi ; Switch stacks

    sti
idle_loop:
    hlt ; Wait for next interrupt

    jmp idle_loop