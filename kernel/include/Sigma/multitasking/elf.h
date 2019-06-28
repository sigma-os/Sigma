#ifndef SIGMA_KERNEL_MULTITASKING_ELF
#define SIGMA_KERNEL_MULTITASKING_ELF

#include <Sigma/common.h>

namespace multitasking::elf
{
    using Elf64_Word = uint32_t;
    using Elf64_Off = uint64_t;
    using Elf64_Xword = uint64_t;
    using Elf64_Addr = uint64_t;

    struct Elf64_Shdr{
	    Elf64_Word sh_name; /* Section name */
        Elf64_Word sh_type; /* Section type */
        Elf64_Xword sh_flags; /* Section attributes */
        Elf64_Addr sh_addr; /* Virtual address in memory */
        Elf64_Off sh_offset; /* Offset in file */
        Elf64_Xword sh_size; /* Size of section */
        Elf64_Word sh_link; /* Link to other section */
        Elf64_Word sh_info; /* Miscellaneous information */
        Elf64_Xword sh_addralign; /* Address alignment boundary */
        Elf64_Xword sh_entsize;
    } __attribute__((packed));

    constexpr uint64_t SHF_WRITE = 0x1;
    constexpr uint64_t SHF_ALLOC = 0x2;
    constexpr uint64_t SHF_EXECINSTR = 0x4;

} // multitasking::elf


#endif