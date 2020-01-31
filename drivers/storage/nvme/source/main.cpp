#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <libsigma/sys.h>
#include <nvme/io_controller.hpp>


int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    auto device_descriptor = devctl(devCtlFindPciClass, 0x1, 0x8, 0x2, 0); // Only search for I/O Controllers
    if(device_descriptor == -1){
        std::cout << "nvme: Couldn't find an I/O controller" << std::endl;
        return 0;
    }

    if(devctl(devCtlClaim, device_descriptor, 0, 0, 0) == -1){
        std::cout << "nvme: Failed to claim controller" << std::endl;
        return 0;
    }

    libsigma_resource_region_t region = {};
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 0, (uint64_t)&region);

    nvme::io_controller controller{region};
    while(1)
        asm("pause");
}