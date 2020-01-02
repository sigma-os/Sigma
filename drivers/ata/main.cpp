#include "ata.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <libsigma/device.h>
#include <iostream>

int main(){
    int fd = open("/dev/sysout", O_WRONLY);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    std::cerr << "ata: Driver is W.I.P." << std::endl;

    auto device_descriptor = devctl(DEVCTL_CMD_FIND_PCI_CLASS, 0x1, 0x1, 0, 0);
    if(device_descriptor == -1){
        std::cout << "ata: Couldn't find IDE controller" << std::endl;
        return 0;
    }

    if(devctl(DEVCTL_CMD_CLAIM, device_descriptor, 0, 0, 0) == -1){
        std::cout << "ata: Failed to claim controller" << std::endl;
    }

    libsigma_resource_region_t regions[4] = {};
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 0, (uint64_t)&regions[0], 0);
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 1, (uint64_t)&regions[1], 0);
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 2, (uint64_t)&regions[2], 0);
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 3, (uint64_t)&regions[3], 0);

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