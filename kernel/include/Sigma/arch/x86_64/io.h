#pragma once

#include <Sigma/common.h>

namespace io
{
    uint8_t inb(uint16_t port);
    void outb(uint16_t port, uint8_t value);

    uint16_t inw(uint16_t port);
    void outw(uint16_t port, uint16_t value);

    uint32_t ind(uint16_t port);
    void outd(uint16_t port, uint32_t value);
} // io


