#!/bin/bash

./build/scripts/mkimage.sh
qemu-system-x86_64 -hda sigma.iso -enable-kvm -monitor stdio -serial file:/dev/stdout -smp 4 -machine q35 -d int