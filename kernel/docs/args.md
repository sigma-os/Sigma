The user can pass args to the kernel via grub.cfg
The syntax is currently very simple and quite fragile
There are 2 things you can do
- Define a bool with the key + space, for example `acpi_trace `
- Defne a string with the key + '=' + value + space, for example `dsdt_override=/boot/dsdt.bin`

## OS Features
- `debug` takes a bool containing the type of way debug info should be sent, current legal values are `serial` to send them over rs232 and `vga` to send them via the normal VGA text mode.
    - `serial` is default

## ACPI Features
- `dsdt_override` which should be a string with the path to the file to read the dsdt from on initrd
- `acpi_trace` is a bool that enables tracing of AML opcodes

## CPU Feature control
All of these features are bools
### Features enabled by default
- `noumip` unconditionally disables initialization of UMIP (User Mode Instruction Prevention)
- `nopcid` will disable pcid, even if it can be enabled
- `noinvpcid` will disable the `invpcid` instruction, note that this will not stop pcid from working without it
- `npsmep` will disable SMEP (Supervisor Mode Execution Prevention)
- `nosmap` will disable SMAP (Supervisor Mode Access Prevention)
- `notme` TME currently is an untested feature, so people are advised to turn it off with this flag, it won't disable it if it has been enabled by BIOS / FW, however it will stop Sigma from enabling it
- `nox2apic` Since Sigma doesn't support Intel VT-d IRQ redirection, it is currently impossible to route IRQs to cpus with an APIC id above 256, if an error pops up about this, pass this option to disable the x2apic

### Features disabled by default
- `tsd` will enable the TSD bit in cr4 which disallows the `rdtsc` instruction in userland, this can make speculative exploits harder to pull off, however many applications use this instruction for valid purposes so it is disabled by default