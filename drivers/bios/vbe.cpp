#include "vbe.hpp"
#include <string>
#include <vector>
#include <utility>
#include <cstring>

rm_cpu* cpu;
std::vector<std::pair<uint16_t, vbe::vbe_mode_info_block>> supported_modes{};
bool initialized;

bool vbe::init(rm_cpu& vcpu){
    cpu = &vcpu;

    vbe_info_block* info = (vbe_info_block*)cpu->data_section;
    strncpy(info->sig, "VBE2", 4);

    rm_regs regs{};
    regs.eax = 0x4f00;
    regs.edi = cpu->data_section_addr;

    cpu->intn(0x10, regs);

    if(regs.eax != 0x4F || regs.eflags & (1 << 0)){
        printf("bios: VBE unsupported\n");
        return false;
    }
        
    printf("bios: Detected VBE Support\n\tSignature: %.4s\n\tRevision: %d.%d\n\tTotal Memory: %dKB\n", info->sig, info->major_ver, info->minor_ver, info->total_mem * 64);

    auto print_bios_string = [](const char* prefix, vbe_far_ptr ptr){
        // Real mode segment:offset pair
        uintptr_t str_phys = ptr.phys();
        uintptr_t str_phys_offset = str_phys & 0xFFF;

        auto* str = (const char*)libsigma_vm_map(0x1000, NULL, (void*)str_phys, PROT_READ, MAP_ANON) + str_phys_offset;
        printf("%s%s\n", prefix, str);
    };

    print_bios_string("\tOEM String: ", info->oem);
    print_bios_string("\tVendor String: ", info->vendor_string);
    print_bios_string("\tProduct Name String: ", info->product_name);
    print_bios_string("\tProduct Revision String: ", info->product_revision);

    uintptr_t modes_phys = info->mode_info.phys();
    uintptr_t modes_offset = modes_phys & 0xFFF;
    
    uint16_t* modes = nullptr;

    if(modes_phys >= cpu->data_section_addr && modes_phys <= cpu->data_section_addr + sizeof(vbe_info_block))
        modes = (uint16_t*)(cpu->data_section + (modes_phys - cpu->data_section_addr));
    else
        modes = (uint16_t*)((uintptr_t)libsigma_vm_map(0x1000, NULL, (void*)modes_phys, PROT_READ, MAP_ANON) + modes_offset);
        
    for(size_t i = 0; modes[i] != 0xFFFF; i++)
        supported_modes.push_back({modes[i], {}});

    for(auto& [mode, mode_info] : supported_modes){
        vbe_mode_info_block* mode_info_block = (vbe_mode_info_block*)cpu->data_section;

        regs.eax = 0x4f01;
        regs.ecx = mode;
        regs.edi = cpu->data_section_addr;

        cpu->intn(0x10, regs);
        if(regs.eax != 0x4F || regs.eflags & (1 << 0)){
            printf("bios: Failed to find mode info for VBE mode 0x%04x\n", mode);
            continue;
        }

        mode_info = *mode_info_block;
    }

    printf("\tSupported Video Modes: \n");
    for(auto& [mode, mode_info] : supported_modes){
        printf("\t\t0x%04x: %dx%d\n", mode, mode_info.width, mode_info.height);
    }

    initialized = true;

    return true;
}

uint16_t vbe::set_mode(uint16_t width, uint16_t height, uint8_t bpp){
    if(!initialized)
        return -1;

    uint16_t mode_to_set = 0;
    for(auto& [mode, mode_info] : supported_modes){
        if(mode_info.width == width && mode_info.height == height && mode_info.bpp == bpp){
            mode_to_set = mode;
        }
    }
    
    if(mode_to_set == 0){
        printf("bios: Couldn't find matching mode for %dx%d %d bpp", width, height, bpp);
        return -1;
    }

    rm_regs regs{};
    regs.eax = 0x4f02;  
    regs.ebx = mode_to_set | (1 << 14); // Mode 0x0104, LFB, Clear screen

    cpu->intn(0x10, regs);
    if(regs.eax != 0x4F || regs.eflags & (1 << 0))
        printf("bios: Failed to set video mode 0x%04x\n", mode_to_set);

    return mode_to_set;
}

vbe::vbe_mode_info_block* vbe::get_mode_info(uint16_t mode){
    for(auto& [search_mode, mode_info] : supported_modes){
        if(search_mode == mode){
            return &mode_info;
        }
    }

    return nullptr;
}

bool vbe::edid_supported(){
    rm_regs regs{};

    regs.eax = 0x4f15; // VBE/DDC
    regs.ebx = 0; // Is Supported

    cpu->intn(0x10, regs);

    if((regs.eax & 0xFFFF) != 0x4F || regs.eflags & (1 << 0))
        return false;

    printf("bios: VBE/DDC Supported, Approx time to retrieve EDID Block: %ds%s\n", (regs.ebx >> 8) & 0xFF, (regs.ebx & (1 << 2)) ? ", Screen blanked during data transfer" : "");

    return true;
}

vbe::edid_block vbe::get_edid_block(){
    edid_block* edid = (edid_block*)cpu->data_section;
    rm_regs regs{};

    regs.eax = 0x4f15; // VBE/DDC
    regs.ebx = 0x01; // Get EDID value
    regs.ecx = 0; // Monitor 0
    regs.edx = 0; // EDID block 0
    regs.edi = cpu->data_section_addr;

    cpu->intn(0x10, regs);

    if(regs.eax != 0x4F || regs.eflags & (1 << 0))
        return {};

    uint8_t check = 0;
    for(uint8_t i = 0; i < 128; i++)
        check += ((uint8_t*)edid)[i];

    if(check != 0)
        printf("bios: EDID Checksum failed\n");

    return *edid;
}

std::pair<uint16_t, uint16_t> vbe::get_display_native_res(vbe::edid_block& edid){
    uint16_t width = 0, height = 0;

    width = edid.detailed_timing_info[0].data[2] + ((edid.detailed_timing_info[0].data[4] & 0xF0) << 4);
    height = edid.detailed_timing_info[0].data[5] + ((edid.detailed_timing_info[0].data[7] & 0xF0) << 4);

    if(!width || !height)
        return {1024, 768}; // Assume 1024x768 as safe resolution

    return {width, height};
}