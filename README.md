# Sigma
The kernel of Sigma OS is located in this repo

It is an x86_64 (semi)POSIX Microkernel but still in early development

## How to build
First configure meson
```
build/scripts/setup_meson.sh
```
Then generate an image
```
build/scripts/mkimage.sh
```
After this a simple ISO should be in the main directory and sigma.bin should be in the build_meson dir

Finally to run QEMU
```
build/scripts/run_qemu.sh
```
