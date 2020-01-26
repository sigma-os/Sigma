#include "bga.hpp"
#include <libdriver/io.hpp>

#include <cstdio>

void bga::write(uint16_t reg, uint16_t val){
    outw(index, reg);
    outw(data, val);
}

uint16_t bga::read(uint16_t reg){
    outw(index, reg);
    return inw(data);
}

bga::bga(){
    // TODO: Get Index, Data, Lfb and don't assume the defaults

    index = bgaIoIndex;
    data = bgaIoData;
    lfb = bgaLfbBase;

    uint16_t version = read(bgaIndexId);

    // Check fixed version prefix
    if((version & 0xFFF0) != bgaId0)
        return; // No BGA controller here

    printf("bga: Detected Controller, version: %d\n", version & 0x000F);

    // Sets bgaIndexXres, bgaIndexYres, bgaIndexBpp to their max values
    write(bgaIndexEnable, bgaGetCaps);

    if(read(bgaIndexXres) > bgaMaxXres)
        max_x = bgaMaxXres;
    else
        max_x = read(bgaIndexXres);

    if(read(bgaIndexYres) > bgaMaxYres)
        max_y = bgaMaxYres;
    else
        max_y = read(bgaIndexYres);

    if(read(bgaIndexBpp) > bgaMaxBpp)
        max_bpp = bgaMaxBpp;
    else
        max_bpp = read(bgaIndexBpp);

    printf("\tMax resolution %dx%d at %d bpp\n", max_x, max_y, max_bpp);

    write(bgaIndexEnable, bgaDisabled);
}

void bga::set_mode(std::pair<uint16_t, uint16_t> res, uint16_t bpp){
    if(res.first > max_x || res.second > max_y || bpp > max_bpp){
        printf("bga: Unsupported mode\n");
        return;
    }

    write(bgaIndexEnable, bgaDisabled);
    write(bgaIndexXres, res.first);
    write(bgaIndexYres, res.second);
    write(bgaIndexBpp, bpp);

    write(bgaIndexEnable, bgaEnabled | bgaLfbEnabled);
}

uintptr_t bga::get_lfb_phys(){
    return lfb;
}