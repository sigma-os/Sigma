The user can pass args to the kernel via grub.cfg
The syntax is currently very simple and quite fragile
There are 2 things you can do
- Define a bool with the key + space, for example `acpi_trace `
- Defne a string with the key + '=' + value + space, for example `dsdt_override=/boot/dsdt.bin`

Options
- `dsdt_override` which should be a string with the path to the file to read the dsdt from on initrd
- `acpi_trace` is a bool that enabled tracing of AML opcodes
- `noumip` is a bool which unconditionally disables initialization of UMIP (User Mode Instruction Prevention)
- `nopcid` will disable pcid, even if it can be enabled
- `noinvpcid` will disable the `invpcid` instruction, note that this will not stop pcid from working without it