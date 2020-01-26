#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "vbe.hpp"

int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    rm_cpu cpu{};

    vbe::init(cpu);

    if(!vbe::edid_supported())
        printf("vbe: EDID unsupported\n");

    auto edid = vbe::get_edid_block();

    auto res = vbe::get_display_native_res(edid);

    printf("vbe: Native display res: %dx%d\n", res.first, res.second);

    uint16_t mode = vbe::set_mode(res.first, res.second, 32);

    printf("vbe: mode: %04x set\n", mode);

    auto* info = vbe::get_mode_info(mode);

    printf("vbe: LFB is at 0x%x\n", info->framebuffer);
    return 0;
}