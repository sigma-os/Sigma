CC = gcc

C_SOURCES = $(wildcard src/*.c)
C_OBJECTS = $(C_SOURCES:.c=.o)

CFLAGS = -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I lib/gnu-efi/inc -I lib/gnu-efi/inc/x86_64 -I lib/gnu-efi/inc/protocol -I include/ -DEFI_FUNCTION_WRAPPER -nostdlib
LDFLAGS = lib/gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o -nostdlib -znocombreloc -T lib/gnu-efi/gnuefi/elf_x86_64_efi.lds -shared -Bsymbolic -L lib/gnu-efi/x86_64/gnuefi -L lib/gnu-efi/x86_64/lib -l:libgnuefi.a -l:libefi.a     
OBJCOPY_FLAGS = -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 

OVMF_CODE = /usr/share/ovmf/x64/OVMF_CODE.fd
OVMF_VARS = /usr/share/ovmf/x64/OVMF_VARS.fd

QEMU_FLAGS = -cpu qemu64 -drive if=pflash,format=raw,unit=0,file=${OVMF_CODE},readonly=on -drive if=pflash,format=raw,unit=1,file=${OVMF_VARS} -net none 

.PHONY: clean deps deps_clean

run: qemu_test_image.iso
	qemu-system-x86_64 ${QEMU_FLAGS} -drive file=$<,if=ide

qemu_test_image.iso: sigma_loader.efi
	dd if=/dev/zero of=tmp.fat bs=512 count=2880
	mformat -i tmp.fat -f 1440 ::
	# mmd -i fat.img ::/EFI
	mcopy -i tmp.fat $< ::/
	mcopy -i tmp.fat build/startup.nsh ::/
	mkdir iso
	cp tmp.fat iso/
	xorriso -as mkisofs -R -f -e tmp.fat -no-emul-boot -o $@ iso
	rm tmp.fat
	rm -rf iso/

sigma_loader.efi: ${C_OBJECTS}
	ld $^ ${LDFLAGS} -o sigma_loader.so
	objcopy ${OBJCOPY_FLAGS} sigma_loader.so $@

deps:
	cd lib/gnu-efi && make

deps_clean: 
	cd lib/gnu-efi && make clean

%.o: %.c
	${CC} $< -c ${CFLAGS} -o $@

clean: deps_clean
	rm -f *.efi
	rm -f *.iso
	rm -f *.img
	rm -f *.so
	rm -f *.a
	rm -f src/*.o
	rm -rf iso
