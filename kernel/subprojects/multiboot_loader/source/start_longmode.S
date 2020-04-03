.code32

.set KERNEL_VMA, 0xffffffff80000000

.section .text
.globl _loader_start
.globl p4_table
_loader_start:
	movl %eax, (_start_multiboot_magic - KERNEL_VMA)
    movl %ebx, (_start_multiboot_info - KERNEL_VMA)

	movl $0x80000000, %eax
	cpuid
	cmpl $0x80000001, %eax
	jb no_long_mode

	movl $0x80000001, %eax
	cpuid
	testl $(1 << 29), %edx
	jz no_long_mode

    lgdt (gdt64_pointer - KERNEL_VMA)

    movl %cr0, %eax
    andl $0x7fffffff, %eax
    movl %eax, %cr0

	movl    %cr4, %eax
	orl     $0x30, %eax
	movl    %eax, %cr4

	movl    $p4_table - KERNEL_VMA, %eax
	mov     %eax, %cr3

	movl    $0xc0000080, %ecx
	rdmsr
	orl     $0x00000101, %eax
	wrmsr
 
	movl    %cr0, %eax
	orl     $0x80000000, %eax
	movl %eax, %cr0
	

    ljmp $0x08, $(_start_64 - KERNEL_VMA)

	cli
	hlt

no_long_mode:
	movb $0x30, (0xb8000)

	cli
	hlt

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
