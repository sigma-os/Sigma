#include <Sigma/proc/elf.h>

#include <Sigma/proc/initrd.h>
#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>

using namespace proc::elf;

static bool load_executable(const char* initrd_filename, auxvals* aux, proc::process::thread* thread, char** ld_out_path, uint64_t base){
    proc::elf::Elf64_Ehdr program_header{};
    if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_header), 0, sizeof(proc::elf::Elf64_Ehdr))){
        printf("[ELF]: Couldn't load file header: %s\n", initrd_filename);
        return false;
    }

    aux->at_phdr = 0;
    aux->at_phent = sizeof(Elf64_Phdr);
    aux->at_phnum = program_header.e_phnum;

    proc::elf::Elf64_Phdr program_section_header{};
    for(uint64_t i = 0; i < program_header.e_phnum; i++){
        if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_section_header), (program_header.e_phoff + (sizeof(proc::elf::Elf64_Phdr) * i)), sizeof(proc::elf::Elf64_Phdr))){
            printf("[ELF]: Couldn't load program header [%s] with index [%d]\n", initrd_filename, i);
            return false;
        }

        if(program_section_header.p_type == proc::elf::pt_interp){ // Path of ld.so
            if(ld_out_path == nullptr) continue;
            size_t ld_path_size = program_section_header.p_filesz + 1;
            char* ld_path = new char[ld_path_size];
            memset(static_cast<void*>(ld_path), 0, ld_path_size);

            if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(ld_path), program_section_header.p_offset, program_section_header.p_filesz)){
                printf("[ELF]: Couldn't read interpreter_name [%s]\n", initrd_filename);
                delete[] ld_path;
                return false;
            }
            
            *ld_out_path = ld_path;
        } else if(program_section_header.p_type == proc::elf::pt_phdr) {
            aux->at_phdr = base + program_section_header.p_vaddr;
        } else if(program_section_header.p_type == proc::elf::pt_load){ // Normal Program Section
            if(program_section_header.p_memsz == 0) continue;


            uint64_t flags = map_page_flags_present | map_page_flags_writable | map_page_flags_user; // TODO: Don't have .text as writable
            //proc::elf::Elf64_Word p_flags = program_section_header.p_flags;
            //if((p_flags & proc::elf::pf_x) == 0) flags |= map_page_flags_no_execute;

            uint64_t n_pages = common::div_ceil(program_section_header.p_memsz, mm::pmm::block_size);
            for(uint64_t j = 0; j < n_pages; j++){
                uint64_t frame = reinterpret_cast<uint64_t>(mm::pmm::alloc_block());
                if(frame == 0){
                    printf("[ELF]: Couldn't allocate physical frames for process\n");
                    return false;
                }
                thread->resources.frames.push_back(frame);

                uint64_t virt = base + program_section_header.p_vaddr + (j * mm::pmm::block_size);
                thread->vmm.map_page(frame, virt, flags);
                memset(reinterpret_cast<void*>(virt & ~(mm::pmm::block_size - 1)), 0, mm::pmm::block_size);
            }

            if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(base + program_section_header.p_vaddr), program_section_header.p_offset, program_section_header.p_filesz)){
                printf("[ELF]: Couldn't read program data [%s]\n", initrd_filename);
                return false;
            }
        }
    }

    aux->at_entry = base + program_header.e_entry;

    return true;
}

static bool check_elf_executable(proc::elf::Elf64_Ehdr* program_header){


    if(program_header->e_ident[proc::elf::ei_mag0] != 0x7f || program_header->e_ident[proc::elf::ei_mag1] != 'E' || program_header->e_ident[proc::elf::ei_mag2] != 'L' || program_header->e_ident[proc::elf::ei_mag3] != 'F'){
        printf("[ELF]: ELF magic not intact for\n");
        return false;
    }

    switch(program_header->e_ident[proc::elf::ei_class]){
        case proc::elf::elfclass32:
            printf("Sigma does not and will not support the IA32e submode for 32bit executables\n");
            return false;

        case proc::elf::elfclass64:
            break;

        default:
            printf("[ELF]: Unknown ELF file class [%x]\n", program_header->e_ident[proc::elf::ei_class]);
            return false;
    }

    switch (program_header->e_ident[proc::elf::ei_data])
    {
    case proc::elf::elfdata2msb:
        printf("[ELF]: No MSB support\n");
        return false;

    case proc::elf::elfdata2lsb:
        break;
    
    default:
        printf("[ELF]: Unknown ELF data type [%x]\n", program_header->e_ident[proc::elf::ei_data]);
        return false;
    }

    switch(program_header->e_ident[proc::elf::ei_osabi]){
        case proc::elf::elfosabi_sysv:
            break;

        default:
            printf("[ELF]: Unknown ELF OSABI [%x]\n", program_header->e_ident[proc::elf::ei_osabi]);
            return false;
    }

    return true;
}

bool proc::elf::start_elf_executable(const char* initrd_filename, proc::process::thread** thread){
    proc::elf::Elf64_Ehdr program_header;
    if(!proc::initrd::read_file(initrd_filename, reinterpret_cast<uint8_t*>(&program_header), 0, sizeof(proc::elf::Elf64_Ehdr))){
        printf("[ELF]: Couldn't load file: %s\n", initrd_filename);
        return false;
    }
    
    check_elf_executable(&program_header);

    // File integrity should be fine now
    switch (program_header.e_type)
    {
    case proc::elf::et_exec:
        // Static Executable File
        {
            proc::process::thread* new_thread = proc::process::create_blocked_thread(nullptr, 0, 0, proc::process::thread_privilege_level::APPLICATION);
            new_thread->vmm = x86_64::paging::paging();

            mm::vmm::kernel_vmm::get_instance().clone_info(new_thread->vmm);

            new_thread->vmm.set_paging_info();

            char* ld_path = nullptr;
            auxvals aux{};
            load_executable(initrd_filename, &aux, new_thread, &ld_path, 0);

            proc::process::expand_thread_stack(new_thread, 10); // Create a stack of 10 pages for the process

            new_thread->context.rsp = new_thread->image.stack_top;
            new_thread->context.rbp = new_thread->context.rsp;
            new_thread->context.cr3 = (new_thread->vmm.get_paging_info() - KERNEL_VBASE);

            if(ld_path == nullptr){
                // Static Executable, No Dynamic loader
                new_thread->context.rip = aux.at_entry;
            } else {
                // Load dynamic shit
                proc::elf::Elf64_Ehdr ld_program_header;
                if(!proc::initrd::read_file(ld_path, reinterpret_cast<uint8_t*>(&ld_program_header), 0, sizeof(proc::elf::Elf64_Ehdr))){
                    printf("[ELF]: Couldn't load file: %s\n", ld_path);
                    return false;
                }
    
                check_elf_executable(&ld_program_header);
                auxvals ld_aux{};
                //TODO: Don't hardcode ld.so offset
                load_executable(ld_path, &ld_aux, new_thread, nullptr, 0x800000000);

                auto push = [&](uint64_t value){
                    new_thread->context.rsp -= sizeof(uint64_t);
                    *(uint64_t*)(new_thread->context.rsp) = value;
                };

                push(0); // Null
                push(0); // Null data
                push(aux.at_phdr);
                push(3); // ph_hdr
                push(aux.at_phent);
                push(4); // ph_ent
                push(aux.at_phnum);
                push(5); // ph_num
                push(aux.at_entry);
                push(9); // entry
                push(0);
                push(0);
                push(0);

                printf("%x, %x, %x, %x\n", aux.at_phdr, aux.at_phent, aux.at_phnum, aux.at_entry);

                new_thread->context.rip = ld_aux.at_entry;

                // Cleanup
                delete[] ld_path;
            }

            mm::vmm::kernel_vmm::get_instance().set();
            *thread = new_thread;
            proc::process::set_thread_state(new_thread, proc::process::thread_state::IDLE);
        }
        break;
    
    default:
        printf("[ELF]: Unknown ELF file type [%x]\n", program_header.e_type);
        return false;
        break;
    }

    return true;
}