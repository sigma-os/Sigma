#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <libsigma/device.h>
#include <iostream>

#include "ahci.hpp"

int main(){
    int fd = open("/dev/sysout", O_WRONLY);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    auto device_descriptor = devctl(DEVCTL_CMD_FIND_PCI_CLASS, 0x1, 0x6, 0, 0);
    if(device_descriptor == -1){
        std::cout << "ahci: Couldn't find a compatible controller" << std::endl;
        return 0;
    }

    if(devctl(DEVCTL_CMD_CLAIM, device_descriptor, 0, 0, 0) == -1){
        std::cout << "ahci: Failed to claim controller" << std::endl;
        return 0;
    }

    libsigma_resource_region_t region = {};
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 5, (uint64_t)&region, 0);

    std::cout << "ahci: Detected ahci controller at: 0x" << std::hex << region.base << ", len: 0x" << region.len << std::endl;
    while(1);
}