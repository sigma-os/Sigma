.code16

.text

.set KERNEL_VMA, 0xffffffff80000000
.set SMP_START, 0x1000
.set SMP_START_SEG, 0x100

.globl trampoline_start
trampoline_start:
    cli
    
    movw $SMP_START_SEG, %ax
    movw %ax, %ds 
    movw %ax, %es 
    movw %ax, %fs 
    movw %ax, %gs 
    movw %ax, %ss 

    lgdt (trampoline_32bit_gdt_pointer - trampoline_start)

    movl %cr0, %eax
    bts $0, %eax // set PE (Protection Enable) bit
    movl %eax, %cr0

    jmp trampoline_flush_segements

trampoline_flush_segements:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    DATA32 ljmp $0x08, $(SMP_START + (trampoline_enter_32 - trampoline_start))

    cli
    hlt

.code32

trampoline_enter_32:
    movb $1, (SMP_START + (trampoline_booted - trampoline_start))
    cli
    hlt


.align 16
trampoline_32bit_gdt_start:
    .word 0 // NULL entry
    .word 0
    .word 0
    .word 0
trampoline_32bit_gdt_code:
    .word 0xffff
    .word 0x0000
    .word 0x9a00
    .word 0x00cf
trampoline_32bit_gdt_data:
    .word 0xffff
    .word 0x0000
    .word 0x9200
    .word 0x00cf
trampoline_32bit_gdt_end:


.align 16
trampoline_32bit_gdt_pointer:
    .word trampoline_32bit_gdt_end - trampoline_32bit_gdt_start - 1
    .long (SMP_START + (trampoline_32bit_gdt_start - trampoline_start)) 



.globl trampoline_stack
trampoline_stack: .quad 0

.globl trampoline_booted
trampoline_booted: .byte 0

.globl trampoline_end
trampoline_end: