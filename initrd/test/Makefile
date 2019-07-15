.PHONY: all clean

all: clean
	nasm -f elf64 test.asm -o test.o
	x86_64-elf-gcc -ffreestanding -fno-pic -no-pie -nostdlib test.o -o test

clean:
	rm -f test test.o