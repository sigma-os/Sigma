#ifndef SIGMA_KERNEL_PROC_ELF
#define SIGMA_KERNEL_PROC_ELF

#include <Sigma/common.h>
#include <Sigma/proc/process.h>
#include <klibcxx/utility.hpp>

namespace proc::elf
{
    using Elf64_Addr = uint64_t;
    using Elf64_Off = uint64_t;
    using Elf64_Half = uint16_t;
    using Elf64_Word = uint32_t;
    using Elf64_Sword = int32_t;
    using Elf64_Xword = uint64_t;
    using Elf64_Sxword = int64_t;

    struct PACKED_ATTRIBUTE Elf64_Ehdr{
        uint8_t e_ident[16];
        Elf64_Half e_type;
        Elf64_Half e_machine;
        Elf64_Word e_version;
        Elf64_Addr e_entry;
        Elf64_Off e_phoff;
        Elf64_Off e_shoff;
        Elf64_Word e_flags;
        Elf64_Half e_ehsize;
        Elf64_Half e_phentsize;
        Elf64_Half e_phnum;
        Elf64_Half e_shentsize;
        Elf64_Half e_shnum;
        Elf64_Half e_shstrndx;
    };

    constexpr uint8_t ei_mag0 = 0;
    constexpr uint8_t ei_mag1 = 1;
    constexpr uint8_t ei_mag2 = 2;
    constexpr uint8_t ei_mag3 = 3;
    constexpr uint8_t ei_class = 4;
    constexpr uint8_t ei_data = 5;
    constexpr uint8_t ei_version = 6;
    constexpr uint8_t ei_osabi = 7;

    constexpr uint8_t elfclass32 = 1;
    constexpr uint8_t elfclass64 = 2;

    constexpr uint8_t elfdata2lsb = 1;
    constexpr uint8_t elfdata2msb = 2;

    constexpr uint8_t elfosabi_sysv = 0;
    // 1 = HP-UX so irrelevant
    constexpr uint8_t elfosabi_standalone = 255;

    constexpr uint16_t et_none = 0; // No file?
    constexpr uint16_t et_rel = 1; // .o file
    constexpr uint16_t et_exec = 2; // normal no extension executable file
    constexpr uint16_t et_dyn = 3; //.so file

    struct PACKED_ATTRIBUTE Elf64_Shdr{
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
    };

    constexpr uint64_t sht_symtab = 2;
    constexpr uint64_t sht_strtab = 3;

    constexpr uint64_t shf_write = 0x1;
    constexpr uint64_t shf_alloc = 0x2;
    constexpr uint64_t shf_execinstr = 0x4;

    struct PACKED_ATTRIBUTE Elf64_Sym {
        Elf64_Word st_name;
        uint8_t st_info;
        uint8_t st_other;
        Elf64_Half st_shndx;
        Elf64_Addr st_value;
        Elf64_Xword st_size;
    };

    constexpr uint8_t stb_local = 0;
    constexpr uint8_t stb_global = 1;
    constexpr uint8_t stb_weak = 2;

    constexpr uint8_t stt_notype = 0;
    constexpr uint8_t stt_object = 1;
    constexpr uint8_t stt_func = 2;
    constexpr uint8_t stt_section = 3;
    constexpr uint8_t stt_file = 4;

    struct PACKED_ATTRIBUTE Elf64_Phdr{
        Elf64_Word p_type;
        Elf64_Word p_flags;
        Elf64_Off p_offset;
        Elf64_Addr p_vaddr;
        Elf64_Addr p_paddr; // Reserved
        Elf64_Xword p_filesz;
        Elf64_Xword p_memsz;
        Elf64_Xword p_align;
    };

    constexpr uint16_t pt_null = 0;
    constexpr uint16_t pt_load = 1;
    constexpr uint16_t pt_dynamic = 2;
    constexpr uint16_t pt_interp = 3;
    constexpr uint16_t pt_note = 4;
    constexpr uint16_t pt_phdr = 6;

    constexpr uint16_t pf_x = 1;
    constexpr uint16_t pf_w = 2;
    constexpr uint16_t pf_r = 4;

    struct auxvals {
        uint64_t at_execfd;
        uint64_t at_phdr;
        uint64_t at_phent;
        uint64_t at_phnum;
        uint64_t at_pagesz;
        uint64_t at_base;
        uint64_t at_flags;
        uint64_t at_entry;
        uint64_t at_notelf;
        uint64_t at_uid;
        uint64_t at_euid;
        uint64_t at_gid;
        uint64_t at_egid;
    };

    struct auxv {
        int a_type;
        union a_un
        {
            uint64_t a_val;
            void* a_ptr;
            void (*a_fnc)();
        };
    };

    bool start_elf_executable(const char* initrd_filename, proc::process::thread** thread, proc::process::thread_privilege_level privilege);
    void map_kernel(boot::boot_protocol& protocol);
    void init_symbol_list(boot::boot_protocol& protocol);
    std::pair<const char*, size_t> get_symbol(uint64_t address); 
} // proc::elf


#endif