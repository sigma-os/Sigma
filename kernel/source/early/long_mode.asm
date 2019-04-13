[bits 32]

section .data
align 0x1000

p4_table: 
    times 0x1000 db 0
p3_table: 
    times 0x1000 db 0
p2_table: 
    times 0x1000 db 0



section .text
align 4

global enter_long_mode
global check_long_mode
extern error

check_long_mode:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .no_long_mode
    ret

.no_long_mode:
    mov al, '4'
    ret



enter_long_mode:
    call set_up_page_tables

    mov eax, cr0
    bt eax, 31
    jc .paging_already_enabled


    mov eax, cr4
    or eax, (CR4_PAE_FLAG | CR4_PSE_FLAG)
    mov cr4, eax
 
    mov eax, p4_table
    mov cr3, eax

    mov ecx, EFER_MSR
    rdmsr
    or eax, EFER_MSR_LONG_MODE_ENABLE
    wrmsr

    mov eax, cr0
    or eax, CR0_PAGING_FLAG
    mov cr0, eax

    ret

.paging_already_enabled:
    mov al, '5'
    call error

set_up_page_tables:
    mov eax, p3_table
    or eax, PAGE_FLAGS_PRESENT_WRITABLE
    mov dword [p4_table], eax

    mov eax, p2_table
    or eax, PAGE_FLAGS_PRESENT_WRITABLE
    mov dword [p3_table], eax

    mov ecx, 0

.map_p2_table:
    mov eax, 0x200000
    mul ecx
    or eax, 0b10000011
    mov [p2_table + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ret

PAGE_FLAGS_PRESENT_WRITABLE equ 0b11

CR0_PAGING_FLAG equ (1 << 31)

CR4_PSE_FLAG equ (1 << 4)
CR4_PAE_FLAG equ (1 << 5)

EFER_MSR equ 0xC0000080

EFER_MSR_LONG_MODE_ENABLE equ (1 << 8)
EFER_MSR_NXE_ENABLE equ (1 << 11)


extern long_mode_start

; Early GDT
global install_early_gdt
install_early_gdt:
    lgdt [gdt64_pointer]
    jmp CODE_SEG:long_mode_start



section .rodata
gdt64:
    dq 0 ; Null Entry
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Kernel Code Segment
    dq (1 << 41) | (1 << 44) | (1 << 47) ; Kernel Data Segment
gdt64_end:
CODE_SEG equ 0x8
DATA_SEG equ 0x10




align 4
gdt64_pointer:
    dw gdt64_end - gdt64 - 1
    dq gdt64



