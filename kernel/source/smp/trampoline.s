.code16

.set KERNEL_VMA, 0xffffffff80000000

.globl trampoline_start
trampoline_start:
    cli
    
    movw $0xb800, %ax
    movw %ax, %ds 
    movw $0, %bx
    movb $0x51, (%bx)

    hlt





.globl trampoline_end
trampoline_end:

.globl trampoline_stack
trampoline_stack: .quad 0
