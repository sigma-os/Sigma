The user can pass args to the kernel via grub.cfg
The syntax is currently very simple and quite fragile
There are 2 things you can do
- Define a bool with the key + space, for example `disable_acpi `
- Defne a string with the key + '=' + value + space, for example `dsdt_override=/boot/dsdt.bin`

Options
- `dsdt_override` which should be a string with the path to the file to read the dsdt from on initrd