#ifndef SIGMA_ARCH_X86_64_IO
#define SIGMA_ARCH_X86_64_IO

#include <Sigma/common.h>

namespace x86_64::io
{
    uint8_t inb(uint16_t port);
    void outb(uint16_t port, uint8_t value);

    uint16_t inw(uint16_t port);
    void outw(uint16_t port, uint16_t value);

    uint32_t ind(uint16_t port);
    void outd(uint16_t port, uint32_t value);
} // x86_64::io

#endif
