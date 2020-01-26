#pragma once

#include <cstdint>
#include <utility>

enum {
    bgaTotalVideoMemMb = 16,
    bga4BppPlaneShift = 4,

    bgaBankAddress = 0xA0000,
    bgaBankSizeKb = 64,

    bgaMaxXres = 2560,
    bgaMaxYres = 1600,
    bgaMaxBpp = 32,

    bgaIoIndex = 0x1CE,
    bgaIoData = 0x1CF,

    bgaIndexId = 0x0,
    bgaIndexXres = 0x1,
    bgaIndexYres = 0x2,
    bgaIndexBpp = 0x3,
    bgaIndexEnable = 0x4,
    bgaIndexBank = 0x5,
    bgaIndexVirtWidth = 0x6,
    bgaIndexVirtHeight = 0x7,
    bgaIndexXoffset = 0x8,
    bgaIndexYoffset = 0x9,
    bgaIndexVideoMemory64k = 0xa,
    bgaIndexDdc = 0xb,

    bgaId0 = 0xB0C0,
    bgaId1 = 0xB0C1,
    bgaId2 = 0xB0C2,
    bgaId3 = 0xB0C3,
    bgaId4 = 0xB0C4,
    bgaId5 = 0xB0C5,

    bgaDisabled = 0x0,
    bgaEnabled = 0x1,
    bgaGetCaps = 0x2,
    bga8bitDax = 0x20,
    bgaLfbEnabled = 0x40,
    bgaNoClearMem = 0x80,

    bgaLfbBase = 0xE0000000
};

class bga {
    public:
    bga();

    void set_mode(std::pair<uint16_t, uint16_t> res, uint16_t bpp);

    uintptr_t get_lfb_phys();

    private:
    void write(uint16_t reg, uint16_t val);
    uint16_t read(uint16_t reg);

    uint16_t index, data;
    uint32_t lfb;

    uint16_t max_x, max_y, max_bpp;
};