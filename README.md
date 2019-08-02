# Sigma
Sigma OS is located in this repo
 
It is an x86_64 (semi)POSIX (semi)Microkernel but still in early development
 
## How to build
Look in [sigma-os/bootstrap](https://github.com/sigma-os/bootstrap)
 
## Features
- Full x86_64 long mode support
- ACPI and AML support with [LAI](https://www.github.com/qword-os/lai)
- APIC interrupt support
- PCI / PCI-Express Configuration Space enumeration
- SMP with scheduler level support
 
## On the microkernel
Sigma does try to have as much drivers and things in userspace(PL3) but it doesn't attempt to move the memory manager and process management / scheduler out of the kernel.
