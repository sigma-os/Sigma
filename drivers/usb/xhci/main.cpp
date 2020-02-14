#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <libsigma/sys.h>

#include "xhci.hpp"

int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    printf("xhci: Starting xHCI driver\n");

    auto device_descriptor = devctl(devCtlFindPciClass, 0xC, 0x3, 0x30, 0);
    if(device_descriptor == -1){
        printf("xhci: Couldn't find a compatible controller\n");
        return 0;
    }

    if(devctl(devCtlClaim, device_descriptor, 0, 0, 0) == -1){
        printf("xhci: Failed to claim controller\n");
        return 0;
    }


    xhci::controller c{device_descriptor};

    return 0;
}