#include <Sigma/proc/elf.h>

#include <Sigma/proc/initrd.h>
#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>

static bool load_static_executable(const char* initrd_filename, proc::process::thread** thread){
    *thread = nullptr;

    proc::elf::Elf64_Ehdr program_header;
    if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_header), 0, sizeof(proc::elf::Elf64_Ehdr))){
        printf("[ELF]: Couldn't load file header: %s\n", initrd_filename);
        return false;
    }

    auto proc_paging = x86_64::paging::paging();
    mm::vmm::kernel_vmm::get_instance().clone_info(proc_paging);
    proc_paging.set_paging_info();

    proc::process::thread* new_thread = proc::process::create_blocked_thread(nullptr, 0, 0, proc::process::thread_privilege_level::APPLICATION); // TODO: Don't assume driver

    proc::elf::Elf64_Phdr program_section_header;
    for(uint64_t i = 0; i < program_header.e_phnum; i++){
        if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_section_header), (program_header.e_phoff + (sizeof(proc::elf::Elf64_Phdr) * i)), sizeof(proc::elf::Elf64_Phdr))){
            printf("[ELF]: Couldn't load program header [%s] with index [%d]\n", initrd_filename, i);
            return false;
        }


        if(program_section_header.p_type == proc::elf::pt_load){
            if(program_section_header.p_memsz == 0) continue;

            uint64_t flags = map_page_flags_present | map_page_flags_writable | map_page_flags_user; // TODO: Don't have .text as writable
            proc::elf::Elf64_Word p_flags = program_section_header.p_flags;
            if((p_flags & proc::elf::pf_x) == 0) flags |= map_page_flags_no_execute;

            uint64_t n_pages = DIV_CEIL(program_section_header.p_memsz, mm::pmm::block_size);
            for(uint64_t j = 0; j < n_pages; j++){
                uint64_t frame = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
                if(frame == 0){
                    printf("[ELF]: Couldn't allocate physical frames for process\n");
                    return false;
                }
                new_thread->resources.frames.push_back(frame);
                proc_paging.map_page(frame, program_section_header.p_vaddr + (j * mm::pmm::block_size), flags);
            }

            if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(program_section_header.p_vaddr), program_section_header.p_offset, program_section_header.p_filesz)){
                printf("[ELF]: Couldn't read program data [%s]\n", initrd_filename);
                return false;
            }
            memset((void*)(program_section_header.p_vaddr + program_section_header.p_filesz), 0, (program_section_header.p_memsz - program_section_header.p_filesz));
        }
    }

    // Todo: actually handle stack and heap correctly
    uint64_t frame = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
    if(frame == 0){
        printf("[ELF]: Couldn't allocate physical frames for process stack\n");
        return false;
    }
    proc_paging.map_page(frame, 0x800000, map_page_flags_present | map_page_flags_writable | map_page_flags_user | map_page_flags_no_execute);

    new_thread->context.rsp = 0x801000;
    new_thread->context.rbp = new_thread->context.rsp;
    new_thread->context.cr3 = (proc_paging.get_paging_info() - KERNEL_VBASE);
    new_thread->context.rip = program_header.e_entry;
    proc::process::set_thread_state(new_thread, proc::process::thread_state::IDLE);

    mm::vmm::kernel_vmm::get_instance().set();

    *thread = new_thread;
    return true;
}

bool proc::elf::start_elf_executable(const char* initrd_filename, proc::process::thread** thread){
    proc::elf::Elf64_Ehdr program_header;
    
    if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_header), 0, sizeof(proc::elf::Elf64_Ehdr))){
        printf("[ELF]: Couldn't load file: %s\n", initrd_filename);
        return false;
    }

    if(program_header.e_ident[proc::elf::ei_mag0] != 0x7f || program_header.e_ident[proc::elf::ei_mag1] != 'E' || program_header.e_ident[proc::elf::ei_mag2] != 'L' || program_header.e_ident[proc::elf::ei_mag3] != 'F'){
        printf("[ELF]: ELF magic not intact for: %s\n", initrd_filename);
        return false;
    }

    switch(program_header.e_ident[proc::elf::ei_class]){
        case proc::elf::elfclass32:
            PANIC("TODO: Implement IA32e submode\n");
            break;

        case proc::elf::elfclass64:
            break;

        default:
            printf("[ELF]: Unknown ELF file class [%x]\n", program_header.e_ident[proc::elf::ei_class]);
            return false;
    }

    switch (program_header.e_ident[proc::elf::ei_data])
    {
    case proc::elf::elfdata2msb:
        PANIC("[ELF]: No MSB support\n");
        break;

    case proc::elf::elfdata2lsb:
        break;
    
    default:
        printf("[ELF]: Unknown ELF data type [%x]\n", program_header.e_ident[proc::elf::ei_data]);
        return false;
        break;
    }

    switch(program_header.e_ident[proc::elf::ei_osabi]){
        case proc::elf::elfosabi_sysv:
            break;

        default:
            printf("[ELF]: Unknown ELF OSABI [%x]\n", program_header.e_ident[proc::elf::ei_osabi]);
            return false;
    }

    // File integrity should be fine now
    switch (program_header.e_type)
    {
    case proc::elf::et_exec:
        // Static Executable File
        {
            proc::process::thread* created_thread = nullptr;
            load_static_executable(initrd_filename, &created_thread);
            *thread = created_thread;
        }
        break;
    
    default:
        printf("[ELF]: Unknown ELF file type [%x]\n", program_header.e_type);
        return false;
        break;
    }

    return true;
}