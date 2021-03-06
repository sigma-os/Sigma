#include "ata.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <libsigma/sys.h>
#include <iostream>

int main(){
    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    std::cerr << "ata: Driver is W.I.P." << std::endl;

    auto device_descriptor = devctl(devCtlFindPciClass, 0x1, 0x1, 0, 0);
    if(device_descriptor == -1){
        std::cout << "ata: Couldn't find IDE controller" << std::endl;
        return 0;
    }

    if(devctl(devCtlClaim, device_descriptor, 0, 0, 0) == -1){
        std::cout << "ata: Failed to claim controller" << std::endl;
    }

    libsigma_resource_region_t regions[4] = {};
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 0, (uint64_t)&regions[0]);
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 1, (uint64_t)&regions[1]);
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 2, (uint64_t)&regions[2]);
    devctl(devCtlGetResourceRegion, device_descriptor, resourceRegionOriginPciBar, 3, (uint64_t)&regions[3]);

    std::pair<uint16_t, uint16_t> ata1_base = ata::isa_ata1_base;

    if(regions[0].base != 0)
        ata1_base.first = regions[0].base;

    if(regions[1].base != 0)
        ata1_base.second = regions[1].base;

    std::cout << "ata: Booting controller 1 with region 0x" << std::hex << ata1_base.first << " : 0x" << ata1_base.second << std::endl;
    ata::controller ata1{ata1_base};
    std::cout << "ata: Initialized controller 1" << std::endl;
    while(1);
}