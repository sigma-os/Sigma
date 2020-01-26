#pragma once

#include "real_mode.hpp"

#include <utility>

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

    struct [[gnu::packed]] edid_block {
        uint8_t fixed_header[8];
        uint16_t manufacturer_id;
        uint16_t manufacturer_product_code;
        uint32_t serial_number;
        uint8_t week_of_manufacture;
        uint8_t year_of_manufacture;
        uint8_t major_rev;
        uint8_t minor_rev;
        uint8_t video_input_param_bitmap;
        uint8_t horizontal_screen_size;
        uint8_t vertical_screen_size;
        uint8_t display_gamma;
        struct {
            uint8_t continous_timings : 1;
            uint8_t prefered_timing_mode : 1;
            uint8_t sRGB_colour_space : 1;
            uint8_t colour_type : 2;
            uint8_t dpms_active_off : 1;
            uint8_t dpms_suspend : 1;
            uint8_t dpms_standby : 1;
        } features;
        uint8_t rg_lsb;
        uint8_t bw_lsb;
        uint8_t red_msb;
        uint8_t red_msb_8;
        uint16_t green_msb_8;
        uint16_t blue_msb_8;
        uint16_t white_msb_8;
        uint8_t established_timing_bitmap[3];
        struct {
            uint8_t x_res;
            struct {
                uint8_t vertical_freq : 6;
                uint8_t aspect_ratio : 2;
            };
        } standard_timing_info[8];
        struct {
            uint8_t data[18];
        } detailed_timing_info[4];
        uint8_t n_extensions;
        uint8_t checksum;
    };
    static_assert(sizeof(edid_block) == 128);


    bool init(rm_cpu& vcpu);

    vbe_mode_info_block* get_mode_info(uint16_t mode);
    uint16_t set_mode(uint16_t width, uint16_t height, uint8_t bpp);

    bool edid_supported();
    edid_block get_edid_block();

    std::pair<uint16_t, uint16_t> get_display_native_res(edid_block& edid);
} // namespace vbe
