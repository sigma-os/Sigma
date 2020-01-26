#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <libsigma/device.h>
#include <nvme/io_controller.hpp>


int main(){
    int sysout_fd = open("/dev/sysout", O_WRONLY);
    dup2(sysout_fd, STDOUT_FILENO);
    dup2(sysout_fd, STDERR_FILENO);

    // Disable buffering for stdout and stderr so debug message show up immediately
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    auto device_descriptor = devctl(DEVCTL_CMD_FIND_PCI_CLASS, 0x1, 0x8, 0x2, 0); // Only search for I/O Controllers
    if(device_descriptor == -1){
        std::cout << "nvme: Couldn't find an I/O controller" << std::endl;
        return 0;
    }

    if(devctl(DEVCTL_CMD_CLAIM, device_descriptor, 0, 0, 0) == -1){
        std::cout << "nvme: Failed to claim controller" << std::endl;
        return 0;
    }

    libsigma_resource_region_t region = {};
    devctl(DEVCTL_CMD_GET_RESOURCE_REGION, device_descriptor, 0, (uint64_t)&region, 0);

    nvme::io_controller controller{region};
    while(1)
        asm("pause");
}