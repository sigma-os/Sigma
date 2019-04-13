AS = nasm
CC = x86_64-elf-gcc
QEMU = qemu-system-x86_64

SOURCE_DIR = source/

CPP_SOURCES = $(shell find ${SOURCE_DIR} -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

ASM_SOURCES = $(shell find ${SOURCE_DIR} -name '*.asm')
ASM_OBJECTS = $(ASM_SOURCES:.asm=.o)

LD_FLAGS = -T build/linker.ld -mno-red-zone -ffreestanding -nostdlib -n

.PHONY: clean run

run: sigma.iso
	${QEMU} -hda sigma.iso -enable-kvm -monitor stdio

bochs: sigma.iso
	bochs -f build/.bochsrc -q

sigma.iso: sigma.bin
	mkdir -p build/iso/boot/grub
	cp build/grub.cfg build/iso/boot/grub/grub.cfg
	cp sigma.bin build/iso/boot/sigma.bin

	grub-mkrescue -o $@ build/iso/

	rm -rf build/iso


sigma.bin: ${CPP_OBJECTS} ${ASM_OBJECTS}
	${CC} ${LD_FLAGS} -o $@ $^ 

%.o: %.asm
	${AS} -f elf64 $< -o $@

clean:
	rm -rf source/*.o
	rm -f sigma.iso sigma.bin