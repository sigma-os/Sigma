#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <libsigma/device.h>

int main(int argc, char* argv[]){
    (void)(argc);
    (void)(argv);
    int fd = open("/dev/sysout", O_WRONLY);
    dup2(fd, STDOUT_FILENO);

    printf("Hello from C land :)");
    fflush(stdout);

    printf("devctl nop: %ld", devctl(DEVCTL_CMD_NOP, 0, 0, 0, 0));
    fflush(stdout);

    uint64_t a = devctl(DEVCTL_CMD_FIND_PCI, 0, 0, 0x1f, 2);
    if(a == UINT64_MAX)
        printf("Failed to find PCI device");
    fflush(stdout);
    
    if(!devctl(DEVCTL_CMD_CLAIM, a, 0, 0, 0))
        printf("Claimed pci device");
    else
        printf("Failed to claim device");
    fflush(stdout);

    return 0;
}