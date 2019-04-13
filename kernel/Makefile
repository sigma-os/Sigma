AS = nasm
GAS = x86_64-elf-as
CC = x86_64-elf-gcc
G++ = x86_64-elf-g++
QEMU = qemu-system-x86_64

SOURCE_DIR = source/

CPP_FLAGS = -m64 -g -Wall -Wextra -Werror -pedantic -std=gnu++17 -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone \
			-fno-stack-protector -I include/ -nostdlib

LD_FLAGS = -T build/linker.ld -mno-red-zone -ffreestanding -nostdlib -n -lgcc

CPP_SOURCES = $(shell find ${SOURCE_DIR} -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

ASM_NASM_SOURCES = $(shell find ${SOURCE_DIR} -name '*.asm')
ASM_NASM_OBJECTS = $(ASM_NASM_SOURCES:.asm=.o)

ASM_GAS_SOURCES = $(shell find ${SOURCE_DIR} -name '*.S')
ASM_GAS_OBJECTS = $(ASM_GAS_SOURCES:.S=.o)

CRTBEGIN_OBJ = $(shell ${G++} ${CPP_FLAGS} -print-file-name=crtbegin.o)
CRTEND_OBJ = $(shell ${G++} ${CPP_FLAGS} -print-file-name=crtend.o)
CRTI_OBJ = source/crti.o
CRTN_OBJ = source/crtn.o


LINK_LIST = ${CRTI_OBJ} ${CRTBEGIN_OBJ} ${ASM_NASM_OBJECTS} ${CPP_OBJECTS} ${CRTEND_OBJ} ${CRTN_OBJ}

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


sigma.bin: ${LINK_LIST} #${CPP_OBJECTS} ${ASM_NASM_OBJECTS} ${ASM_GAS_OBJECTS}
	${CC} ${LD_FLAGS} -o $@ $^

%.o: %.asm
	${AS} -f elf64 $< -o $@

%.o: %.S
	${GAS} $< -o $@

%.o: %.cpp
	${G++} -c $< -o $@ ${CPP_FLAGS}

clean:
	rm -rf source/*.o source/early/*.o
	rm -f sigma.iso sigma.bin