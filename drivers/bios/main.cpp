#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "vbe.hpp"

int main(){
    int sysout_fd = open("/dev/sysout", O_WRONLY);
    dup2(sysout_fd, STDOUT_FILENO);
    dup2(sysout_fd, STDERR_FILENO);

    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    rm_cpu cpu{};

    vbe::init(cpu);

    uint16_t mode = vbe::set_mode(1920, 1080, 32);

    printf("bios: VBE set mode: %04x\n", mode);

    auto* info = vbe::get_mode_info(mode);

    printf("bios: LFB %p\n", (void*)(info->framebuffer));
    return 0;
}