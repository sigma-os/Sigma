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
    movl %cr4, %eax
    bts $4, %eax // Set PSE (Page Size Extension)
    bts $5, %eax // Set PAE (Physical Address Extension
    mov %eax, %cr4

    lgdt (SMP_START + (trampoline_gdt64_pointer - trampoline_start)) 

    mov $((SMP_START + (pml4_table - trampoline_start))), %eax
    mov %eax, %cr3

    movl $0xc0000080, %ecx
    rdmsr
    bts $8, %eax // Set Long Mode Enable Bit
    wrmsr

    movl %cr0, %eax
    bts $31, %eax // set PG (Paging) bit
    movl %eax, %cr0

    ljmp $0x08, $(SMP_START + (trampoline_enter_64 - trampoline_start))

    cli
    hlt



.code64

.globl _smp_kernel_early
trampoline_enter_64:
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    movabsq $_smp_kernel_early, %rax
    jmp *%rax
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

.align 16
trampoline_gdt64:
    .quad 0
    .quad (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)

trampoline_gdt64_end:

.align 16
trampoline_gdt64_pointer:
    .word trampoline_gdt64_end - trampoline_gdt64 - 1
    .quad (SMP_START + (trampoline_gdt64 - trampoline_start)) 

.align 0x1000
pml4_table:
	.quad ((SMP_START + (pdpt_table - trampoline_start)) + 3)
	.fill 510,8,0
	.quad ((SMP_START + (pdpt_table - trampoline_start)) + 3)

pdpt_table:
	.quad ((SMP_START + (pd_table - trampoline_start)) + 3)
	.fill 509,8,0
	.quad ((SMP_START + (pd_table - trampoline_start)) + 3)
	.fill 1,8,0

pd_table:
	.quad 0x0000000000000083
	.quad 0x0000000000200083
	.fill 510,8,0

.globl trampoline_end
trampoline_end: