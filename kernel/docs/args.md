The user can pass args to the kernel via grub.cfg
The syntax is currently very simple and quite fragile
There are 2 things you can do
- Define a bool with the key + space, for example `acpi_trace `
- Defne a string with the key + '=' + value + space, for example `dsdt_override=/boot/dsdt.bin`

Options
- `dsdt_override` which should be a string with the path to the file to read the dsdt from on initrd
- `acpi_trace` is a bool that enables tracing of AML opcodes
- `noumip` is a bool which unconditionally disables initialization of UMIP (User Mode Instruction Prevention)
- `nopcid` will disable pcid, even if it can be enabled
- `noinvpcid` will disable the `invpcid` instruction, note that this will not stop pcid from working without it
- `nosmep` will disable SMEP (Supervisor Mode Execution Prevention)
- `nosmap` will disable SMAP (Supervisor Mode Access Prevention)
- `enable_tsd` will enable the TSD bit in cr4 which disallows the `rdtsc` instruction in userland, this can make speculative exploits harder to pull off, however many applications use this instruction for valid purposes so it is disabled by default
- `notme` TME like SMAP currently is an untested feature, so people are advised to turn it off with this flag, it won't disable it if it has been enabled by BIOS / FW, however it will stop Sigma from enabling it
- `nox2apic` Since Sigma doesn't support Intel VT-d, it is currently impossible to route IRQs to cpus with an APIC id above 256, if an error pops up about this, pass this option to disable the x2apic