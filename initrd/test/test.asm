[section .text]
global _start
_start:
    mov al, byte [t]
    inc al
    mov byte [t], al
    jmp _start

[section .data]
t: db 0