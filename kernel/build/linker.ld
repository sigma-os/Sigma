OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x0000000000100000; /* 1 MEG */
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

ENTRY(_start)

SECTIONS {
	. = KERNEL_VMA;

	_kernel_start = .;

    .boot : AT(ADDR(.boot) - (KERNEL_VMA - KERNEL_LMA)) {
        _kernel_boot = .;
        *(.multiboot)
    }

	.text : AT(ADDR(.text) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_code = .;
		*(.text*)
		*(.gnu.linkonce.t*)
		. = ALIGN(0x1000);
	}

	.rodata : AT(ADDR(.rodata) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_rodata = .;
		*(.rodata*)
		*(.gnu.linkonce.r*)

		. = ALIGN(8);
		__CTOR_LIST__ = .;
		*(.ctors)
		__CTOR_END__ = .;
		
		. = ALIGN(8);
		__DTOR_LIST__ = .;
		*(.dtors)
		__DTOR_END__ = .;
		
		. = ALIGN(8);
		__INIT_LIST__ = .;
		KEEP (*(.init_array))
		__INIT_END__ = .;
		
		. = ALIGN(8);
		__FINI_LIST__ = .;
		KEEP (*(.fini_array))
		__FINI_END__ = .;

		. = ALIGN(0x1000);
	}

	.data : AT(ADDR(.data) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_data = .;
		*(.data*)
		*(.gnu.linkonce.d*)
		. = ALIGN(0x1000);
	}

	.eh_frame : AT(ADDR(.eh_frame) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_eh_frame = .;
		*(.eh_frame)
		*(.eh_frame_hdr)
		. = ALIGN(0x1000);
	}

	_edata = .;
	
	.bss : AT(ADDR(.bss) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_bss = .;
		*(.bss*)
		*(.gnu.linkonce.b*)
		*(COMMON)
		. = ALIGN(0x1000);
	}
	
	_kernel_end = .;

	/DISCARD/ : {
		*(.comment)
		*(note.*)
	}
}