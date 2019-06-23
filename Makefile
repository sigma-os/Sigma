.PHONY: clean lint run bochs sigma.bin

run: sigma.iso
	qemu-system-x86_64 -hda sigma.iso -enable-kvm -monitor stdio -serial file:/dev/stdout -smp 4 -machine q35 -d int

bochs: sigma.iso
	mkdir -p build/log
	bochs -f build/.bochsrc -q

init:
	meson . build_meson --cross-file ./build/meson_cross.ini

sigma.iso: sigma.bin
	mkdir -p build/iso/boot/grub
	cp build/grub.cfg build/iso/boot/grub/grub.cfg
	cp build_meson/sigma.bin build/iso/boot/sigma.bin

	grub-mkrescue -o sigma.iso build/iso/

	rm -rf build/iso

sigma.bin:
	cd build_meson/; ninja
	cp build_meson/sigma.bin .

clean:
	cd build_meson/; ninja clean

lint:
	cppcheck --enable=warning,performance,information,style,portability,missingInclude . -Iinclude/ -j4 --platform=unix64 --std=posix --language=c++ -q -v