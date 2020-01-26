#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "bga.hpp"


int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);


    bga controller{};

    controller.set_mode({1024, 768}, 32);

    printf("bga: LFB at %p", (void*)(controller.get_lfb_phys()));

    return 0;
}