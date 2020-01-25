#pragma once

#include "real_mode.hpp"

namespace vbe
{
    struct [[gnu::packed]] vbe_far_ptr {
        uint16_t off;
        uint16_t seg;

        uint32_t phys() { return (seg << 4) + off; }
    };

    struct [[gnu::packed]] vbe_info_block {
        char sig[4];
        uint8_t minor_ver;
        uint8_t major_ver;
        vbe_far_ptr oem;
        uint32_t cap;
        vbe_far_ptr mode_info;
        uint16_t total_mem;
        uint16_t software_revision;
        vbe_far_ptr vendor_string;
        vbe_far_ptr product_name;
        vbe_far_ptr product_revision;
        uint8_t reserved[222];
        uint8_t oem_data[256];
    };

    struct [[gnu::packed]] vbe_mode_info_block {
        uint16_t attr;
        uint8_t window_a;
        uint8_t window_b;
        uint16_t granularity;
        uint16_t window_size;
        uint16_t seg_a, seg_b;
        uint32_t win_func_ptr;
        uint16_t pitch, width, height;
        uint8_t w_char, y_char;
        uint8_t planes, bpp, banks, mmodel, bank_size, image_pages, reserved, red_mask, red_pos, green_mask, green_pos, blue_mask, blue_pos, reserved_mask, reserved_pos, colour_attr;
        uint32_t framebuffer;
        uint32_t off_screen_mem_off;
        uint16_t off_screen_mem_size;
        uint8_t reserved_0[206];
    };


    bool init(rm_cpu& vcpu);

    vbe_mode_info_block* get_mode_info(uint16_t mode);
    uint16_t set_mode(uint16_t width, uint16_t height, uint8_t bpp);
} // namespace vbe
