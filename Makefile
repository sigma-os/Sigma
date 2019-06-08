AS = nasm
GAS = x86_64-elf-as
CC = x86_64-elf-gcc
G++ = x86_64-elf-g++
QEMU = qemu-system-x86_64

SOURCE_DIR = source/

EXTRA_WARNING_FLAGS = -Wreorder

FLAGS_C_COMMON = -m64 -march=x86-64 -fno-PIC -Wall -Wextra -Werror -ffreestanding -nostdlib \
				 -mcmodel=kernel -I include/ -fno-stack-protector -g -O2 \
				 -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-sse4 -mno-sse4.1 -mno-sse4.2 -mno-sse4a

CPP_FLAGS = ${FLAGS_C_COMMON} ${EXTRA_WARNING_FLAGS} -std=c++17 -mno-red-zone -fno-exceptions -fno-rtti -fuse-cxa-atexit

LD_FLAGS = -nostdlib -mcmodel=kernel -fno-PIC -no-pie -Wl,--build-id=none -Wl,-z,max-page-size=0x1000,-n,-T,./build/linker.ld


CPP_SOURCES = $(shell find ${SOURCE_DIR} -name '*.cpp')
CPP_OBJECTS = $(CPP_SOURCES:.cpp=.o)

ASM_NASM_SOURCES = $(shell find ${SOURCE_DIR} -name '*.asm')
ASM_NASM_OBJECTS = $(ASM_NASM_SOURCES:.asm=.o)

ASM_GAS_SOURCES = $(shell find ${SOURCE_DIR} -name '*.s')
ASM_GAS_OBJECTS = $(ASM_GAS_SOURCES:.S=.o)

CRTBEGIN_OBJ = $(shell ${CC} ${CPP_FLAGS} -print-file-name=crtbegin.o)
CRTEND_OBJ = $(shell ${CC} ${CPP_FLAGS} -print-file-name=crtend.o)
CRTI_OBJ = source/crti.o
CRTN_OBJ = source/crtn.o


LINK_LIST = ${CRTI_OBJ} ${CRTBEGIN_OBJ} ${ASM_GAS_OBJECTS} ${ASM_NASM_OBJECTS} ${CPP_OBJECTS} ${CRTEND_OBJ} ${CRTN_OBJ}

.PHONY: clean run bochs lint

run: sigma.iso
	${QEMU} -hda sigma.iso -enable-kvm -monitor stdio -serial file:/dev/stdout -smp 4 -machine q35 -d int

bochs: sigma.iso
	bochs -f build/.bochsrc -q

lint: 
	cppcheck --enable=warning,performance,information,style,portability,missingInclude . -Iinclude/ -j4 --platform=unix64 --std=posix --language=c++ -q -v

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

%.o: %.s
	${GAS} $< -o $@

%.o: %.S
	${GAS} $< -o $@

%.o: %.cpp
	${G++} -c $< -o $@ ${CPP_FLAGS}

clean:
	rm -rf $(shell find ${SOURCE_DIR} -name '*.o')
	rm -f sigma.iso sigma.bin