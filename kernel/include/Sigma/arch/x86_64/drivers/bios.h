#ifndef SIGMA_KERNEL_BIOS
#define SIGMA_KERNEL_BIOS

#include <Sigma/common.h>

namespace x86_64::bios
{
    constexpr uint64_t probable_ebda_location = (0x80000 + KERNEL_VBASE);

    constexpr uint64_t ebda_end = (0x9FFFF + KERNEL_VBASE);

    constexpr uint64_t rom_space_location = (0xC8000 + KERNEL_VBASE);
    constexpr uint64_t rom_space_end = (0xFFFFF + KERNEL_VBASE);
    

    constexpr uint64_t bios_reset_vector = 0x467;

    class bios {
        public:
            static void* get_ebda_addr(){
                uint16_t* ebda_pointer = reinterpret_cast<uint16_t*>(0x041E + KERNEL_VBASE);
                uint64_t ebda = (*ebda_pointer << 4);

                if(ebda != 0) return reinterpret_cast<void*>(ebda + KERNEL_VBASE);
                else return reinterpret_cast<void*>(x86_64::bios::probable_ebda_location);
            }
    };
} // x86_64::bios



#endif