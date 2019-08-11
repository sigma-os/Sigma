[section .text]
global _start
_start:
    mov rax, 0
    mov rbx, t
    int 249

    mov rax, 2
    int 249

    mov rax, 0
    mov rbx, d
    int 249

.loop:
    jmp .loop

[section .data]
t: db 'Hallo Wereld', 0
d: db 'Excuse me WTF', 0