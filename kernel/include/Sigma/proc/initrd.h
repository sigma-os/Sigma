#ifndef SIGMA_KERNEL_INITRD
#define SIGMA_KERNEL_INITRD

#include <Sigma/common.h>
#include <Sigma/types/linked_list.h>

namespace proc::initrd
{
    struct tar_header{
        char filename[100];
        char mode[8];
        char uid[8];
        char gid[8];
        char size[12];
        char mtime[12];
        char chksum[8];
        char typeflag;
    };

    // Initrd is a simple TAR archive
    void init(uint64_t address, uint64_t size);
    bool read_file(const char* file_name, uint8_t* buf, uint64_t offset, uint64_t size);
    size_t get_size(const char* file_name);
} // namespace proc::initrd


#endif