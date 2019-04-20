#ifndef SIGMA_KERNEL_IST
#define SIGMA_KERNEL_IST

#include <Sigma/common.h>

namespace x86_64::ist
{
    struct table {
        public:
        table(): ist1(0), ist2(0), ist3(0), ist4(0), ist5(0), ist6(0), ist7(0){}

        uint64_t ist1;
        uint64_t ist2;
        uint64_t ist3;
        uint64_t ist4;
        uint64_t ist5;
        uint64_t ist6;
        uint64_t ist7;
    } __attribute__((packed));
} // x86_64::ist

#endif