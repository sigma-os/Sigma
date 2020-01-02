#pragma once

#include <stdint.h>

constexpr uint32_t bswap32(uint32_t dword) {
    return (uint32_t)((dword>>24) & 0xFF)
        | ((dword<<8) & 0xFF0000)
        | ((dword>>8)&0xFF00)
        | ((dword<<24)&0xFF000000);
}