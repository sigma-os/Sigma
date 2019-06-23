# Sigma
The kernel of Sigma OS is located in this repo

It is an x86_64 (semi)POSIX Microkernel but still in early development

## How to build
First configure meson
```
$ make init
```
Then generate an image
```
make sigma.iso
```
After this a simple ISO should be in the main directory

Finally to run QEMU
```
make run
```
