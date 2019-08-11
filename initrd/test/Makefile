.PHONY: all clean

all: clean
	nasm -f elf64 test.asm -o test.o
	gcc -m64 -ffreestanding -fno-pic -no-pie -nostdlib -nostartfiles -nostdinc test.o -o test

clean:
	rm -f test test.o