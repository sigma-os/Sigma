#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <libsigma/sys.h>
#include <iostream>

#include "ahci.hpp"

int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    auto device_descriptor = devctl(devCtlFindPciClass, 0x1, 0x6, 0x1, 0);
    if(device_descriptor == -1){
        std::cout << "ahci: Couldn't find a compatible controller" << std::endl;
        return 0;
    }

    if(devctl(devCtlClaim, device_descriptor, 0, 0, 0) == -1){
        std::cout << "ahci: Failed to claim controller" << std::endl;
        return 0;
    }

    libsigma_resource_region_t region = {};
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 5, (uint64_t)&region);

    ahci::controller controller{region.base, region.len};
    while(1)
        asm("pause");
}