#!/bin/bash

cd build_meson
ninja
cd ..

mkdir -p build/iso/boot/grub
cp build/grub.cfg build/iso/boot/grub/grub.cfg
cp build_meson/sigma.bin build/iso/boot/sigma.bin

grub-mkrescue -o sigma.iso build/iso/

rm -rf build/iso