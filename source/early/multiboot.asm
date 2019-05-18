MULTIBOOT2_HEADER_MAGIC equ 0xE85250D6
MULTIBOOT2_ARCHITECTURE_i386 equ 0

[section .multiboot]
align 4
multiboot_header:
    dd MULTIBOOT2_HEADER_MAGIC ; MAGIC
    dd MULTIBOOT2_ARCHITECTURE_i386 ; ARCHITECTURE
    dd (multiboot_header_end - multiboot_header) ; LENGTH
    dd 0x100000000 - (MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_i386 + (multiboot_header_end - multiboot_header)) ; CHECKSUM


    ; Tags
    dw 6; Type: Page Align Modules
    dw 0; Flags: 0
    dd 8; Size: 8

    ; End Tag
    dw 0 ; Type
    dw 0 ; Flags
    dd 8 ; Size
multiboot_header_end: