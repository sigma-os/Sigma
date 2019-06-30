.PHONY: build clean lint run bochs init sigma.bin initrd.tar sigma.iso

all: run

sigma.bin:
	rm -f sigma.bin kernel/sigma.bin
	cd kernel/; make sigma.bin
	cp kernel/sigma.bin .

initrd.tar:
	rm -f initrd.tar
	./initrd/build.sh

sigma.iso: sigma.bin initrd.tar
	mkdir -p build/iso/boot/grub
	cp kernel/build/grub.cfg build/iso/boot/grub/grub.cfg
	cp sigma.bin build/iso/boot/sigma.bin
	cp initrd.tar build/iso/boot/initrd.tar

	grub-mkrescue -o sigma.iso build/iso/

	rm -rf build/iso

build: sigma.iso

init:
	cd kernel/; make init

clean:
	rm -f sigma.bin sigma.iso initrd.tar
	cd kernel/; make clean

lint:
	cd kernel/; make lint

run: sigma.iso
	qemu-system-x86_64 -hda sigma.iso -enable-kvm -monitor stdio -serial file:/dev/stdout -smp 4 -machine q35 -d int

bochs: sigma.iso
	mkdir -p kernel/build/log
	bochs -f kernel/build/.bochsrc -q
