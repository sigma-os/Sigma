bits 64

section .text

global acquire
acquire:
    push rbp
    mov rbp, rsp

    lock bts word [rdi], 0
    jnc .acquired

.retry:
    pause

    bt word [rdi], 0
    jc .retry

    lock bts word [rdi], 0
    jc .retry

.acquired:
    mov rsp, rbp
    pop rbp
    ret

global release
release:
    push rbp
    mov rbp, rsp

    lock btr word [rdi], 0

    mov rsp, rbp
    pop rbp
    ret