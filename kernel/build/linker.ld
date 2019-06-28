OUTPUT_FORMAT(elf64-x86-64)

KERNEL_LMA = 0x0000000000100000; /* 1 MEG */
KERNEL_VMA = 0xffffffff80000000 + KERNEL_LMA;

ENTRY(_loader_start)

SECTIONS {
	. = KERNEL_VMA;

	_kernel_start = .;

    .boot : AT(ADDR(.boot) - (KERNEL_VMA - KERNEL_LMA)) {
        _kernel_boot = .;
        *(.loader_header*)
		. = ALIGN(0x1000);
    }

	.text : AT(ADDR(.text) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_code = .;
		*(.text*)
		*(.gnu.linkonce.t*)

		. = ALIGN(0x1000);
	}

	.init : AT(ADDR(.init) - (KERNEL_VMA - KERNEL_LMA)) {
		*(.init_start)
		*(.init)
		*(.init_end)
	}

	.fini : AT(ADDR(.fini) - (KERNEL_VMA - KERNEL_LMA)) {
		*(.fini_start)
		*(.fini)
		*(.fini_end)
	}

	.ctors : AT(ADDR(.ctors) - (KERNEL_VMA - KERNEL_LMA)) {
    	/* gcc uses crtbegin.o to find the start of
       	the constructors, so we make sure it is
       	first.  Because this is a wildcard, it
       	doesn't matter if the user does not
       	actually link against crtbegin.o; the
       	linker won't look for a file to match a
       	wildcard.  The wildcard also means that it
       	doesn't matter which directory crtbegin.o
       	is in.  */
    	KEEP (*crtbegin.o(.ctors))
    	KEEP (*crtbegin?.o(.ctors))
    	/* We don't want to include the .ctor section from
       	the crtend.o file until after the sorted ctors.
       	The .ctor section from the crtend file contains the
       	end of ctors marker and it must be last */
    	KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    	KEEP (*(SORT(.ctors.*)))
    	KEEP (*(.ctors))
  	}
  	.dtors : AT(ADDR(.dtors) - (KERNEL_VMA - KERNEL_LMA))
  	{
    	KEEP (*crtbegin.o(.dtors))
    	KEEP (*crtbegin?.o(.dtors))
    	KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    	KEEP (*(SORT(.dtors.*)))
    	KEEP (*(.dtors))
  	}

	.rodata : AT(ADDR(.rodata) - (KERNEL_VMA - KERNEL_LMA)) {
		_kernel_rodata = .;
		*(.rodata*)
		*(.gnu.linkonce.r*)

		/*. = ALIGN(0x1000);
		__CTOR_LIST__ = .;
		*(.ctors)
		__CTOR_END__ = .;

		. = ALIGN(0x1000);
		__DTOR_LIST__ = .;
		*(.dtors)
		__DTOR_END__ = .;*/
		
		. = ALIGN(0x1000);
		__INIT_LIST__ = .;
		KEEP (*(.init_array))
		__INIT_END__ = .;
		
		. = ALIGN(0x1000);
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