[section .text]
global _start
_start:
    mov rax, 0
    mov rbx, t
    int 249

    mov rax, 3
    mov rbx, 0
    mov rcx, 0
    mov rdx, 10
    int 249

    mov rax, 5
    mov rbx, 0x1000
    mov rcx, 0x4000
    mov rdx, 0xFF
    mov rsi, 0xFF
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