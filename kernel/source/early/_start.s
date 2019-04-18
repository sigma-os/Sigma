.code32

.set KERNEL_VMA, 0xffffffff80000000

.section .text
.globl _start
_start:
	//leal (_start_multiboot_magic - KERNEL_VMA), %ecx
    movl %eax, (_start_multiboot_magic - KERNEL_VMA)
	//leal (_start_multiboot_info - KERNEL_VMA), %ecx
    movl %ebx, (_start_multiboot_info - KERNEL_VMA)

    lgdt (gdt64_pointer - KERNEL_VMA)

    movl %cr0, %eax
    andl $0x7fffffff, %eax
    movl %eax, %cr0

	movl    %cr4, %eax
	orl     $0x30, %eax
	movl    %eax, %cr4

	movl    $(p4_table - KERNEL_VMA), %eax
	mov     %eax, %cr3

	movl    $0xc0000080, %ecx
	rdmsr
	orl     $0x00000101, %eax
	wrmsr
 
	movl    %cr0, %eax
	orl     $0x80000000, %eax
	movl %eax, %cr0
	

    ljmp $0x08, $(_start_64 - KERNEL_VMA)

.data
.align 0x1000
p4_table:
	.quad p3_table - KERNEL_VMA + 3
	.fill 510,8,0
	.quad p3_table - KERNEL_VMA + 3

p3_table:
	.quad p2_table - KERNEL_VMA + 3
	.fill 509,8,0
	.quad p2_table - KERNEL_VMA + 3
	.fill 1,8,0

p2_table:
	.quad 0x0000000000000083
	.quad 0x0000000000200083
	.fill 510,8,0

.globl _start_multiboot_info
_start_multiboot_info: .fill 1,4,0
.globl _start_multiboot_magic
_start_multiboot_magic: .fill 1,4,0


.section .rodata
.align 16
gdt64:
    .quad 0
    .quad (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)

gdt64_end:

.align 16
gdt64_pointer:
    .word gdt64_end - gdt64 - 1
    .quad (gdt64 - KERNEL_VMA)


.section .text
.code64
.globl _start_64
.globl long_mode_start
_start_64:
    movabsq $long_mode_start, %rax
    jmp *%rax
