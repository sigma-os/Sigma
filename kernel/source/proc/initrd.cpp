#include <Sigma/proc/initrd.h>
#include <klibc/stdio.h>
#include <Sigma/misc/misc.h>
#include <Sigma/types/vector.h>
#include <klibcxx/mutex.hpp>

static misc::lazy_initializer<types::vector<proc::initrd::tar_header*>> headers;

static uint64_t get_header_number(const char* in){
    size_t size = 0;
    uint64_t count = 1;

    for(uint64_t j = 11; j > 0; j--, count *= 8){
        size += ((in[j - 1] - '0') * count);
    }

    return size;
}

static std::mutex initrd_lock{};

void proc::initrd::init(uint64_t address, uint64_t size){
    std::lock_guard lock{initrd_lock};
    
    headers.init();
    for(uint64_t i = 0; i < size; i++){
        auto* header = reinterpret_cast<proc::initrd::tar_header*>(address);

        if(header->filename[0] == '\0') break; // Invalid header
        uint64_t entry_size = get_header_number(header->size);

        headers->push_back(header);

        address += ((entry_size / 512) + 1) * 512;
        if(entry_size % 512) address += 512;
    }
}

bool proc::initrd::read_file(const char* file_name, uint8_t* buf, uint64_t offset, uint64_t size){
    if((size + offset) > proc::initrd::get_size(file_name) + 1)
        return false; // Yeah lol no, that would be a bit *too* easy wouldn't it

    std::lock_guard lock{initrd_lock};
    for(auto* header : *headers){
        if(strcmp(header->filename, file_name) == 0){
            // Found it
            void* data = static_cast<void*>(static_cast<uint8_t*>(static_cast<void*>(header)) + 512 + offset);
            
            memcpy(static_cast<void*>(buf), data, size);

            return true;
        }
    }
    return false;
}

size_t proc::initrd::get_size(const char* file_name){
    std::lock_guard lock{initrd_lock};
    for(auto* header : *headers){
        if(strcmp(header->filename, file_name) == 0){
            return get_header_number(header->size);
        }
    }

    return 0;
}