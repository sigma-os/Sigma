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

    return 0;
}