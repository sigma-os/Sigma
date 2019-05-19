#ifndef SIGMA_KERNEL_PANIC
#define SIGMA_KERNEL_PANIC

namespace Sigma::panic {
    void panic_m(const char* message, const char* file, const char* func, int line);
}

#endif