KERNEL_PBASE = 0x0000000000100000;
KERNEL_VBASE = 0xffffffff80000000;

ENTRY(_loader_start)

SECTIONS {
    . = KERNEL_VBASE + KERNEL_PBASE;

    _kernel_start = .;
    .boot : AT(ADDR(.boot) - KERNEL_VBASE) {
        *(.loader_header*)
    }

    . = ALIGN(0x1000);

    .text : AT(ADDR(.text) - KERNEL_VBASE) {
        *(.text*)

        *(.init_start)
		*(.init)
		*(.init_end)

        *(.fini_start)
		*(.fini)
		*(.fini_end)
    }

    . = ALIGN(0x1000);

    .ctors : AT(ADDR(.ctors) - KERNEL_VBASE) {
    	KEEP (*crtbegin.o(.ctors))
    	KEEP (*crtbegin?.o(.ctors))
    	KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    	KEEP (*(SORT(.ctors.*)))
    	KEEP (*(.ctors))
  	}
  	.dtors : AT(ADDR(.dtors) - KERNEL_VBASE)
  	{
    	KEEP (*crtbegin.o(.dtors))
    	KEEP (*crtbegin?.o(.dtors))
    	KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    	KEEP (*(SORT(.dtors.*)))
    	KEEP (*(.dtors))
  	}

    . = ALIGN(0x1000);

    .rodata : AT(ADDR(.rodata) - KERNEL_VBASE) {
        *(.rodata*)

        KEEP(*(.init_array))
        KEEP(*(.fini_array))
    }

    . = ALIGN(0x1000);

    .data : AT(ADDR(.data) - KERNEL_VBASE) {
        *(.data*)

        *(.eh_frame)
		*(.eh_frame_hdr)
    }

    .bss : AT(ADDR(.bss) - KERNEL_VBASE) {
        *(.bss*)
    }

    _kernel_end = .;
}