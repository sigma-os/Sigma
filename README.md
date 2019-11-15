# Sigma :rocket:
Sigma OS is located in this repo
 
It is an x86_64 (semi)POSIX (semi)Microkernel but still in early development
 
## How to build
Look in [sigma-os/bootstrap](https://github.com/sigma-os/bootstrap)
 
## Features
- Full x86_64 long mode support
- Support for a lot of x86(\_64) minor features (e.g. SMEP, [SMAP](https://en.wikipedia.org/wiki/Supervisor_Mode_Access_Prevention), UMIP and PCID)
- ACPI and AML support with [lai](https://www.github.com/qword-os/lai)
- APIC support + LAPIC timer for scheduling IRQs
- PCI / PCI-Express Configuration space enumeration
- SMP with scheduler level support
- Dynamically linked userland backed by [mlibc](https://www.github.com/managarm/mlibc)
 
## Notes

- Sigma does try to have as much drivers and things in userspace(PL3) but it doesn't attempt to move the memory manager and process management / scheduler out of the kernel.

- When running vbox use the `ich9` chipset, if you must use the `piix3` chipset you need to explicitly enable the HPET with this command `VBoxManage modifyvm <your vm name> --hpet on`
