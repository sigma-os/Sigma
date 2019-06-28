#ifndef SIGMA_MULTIBOOT_LOADER_COMMON
#define SIGMA_MULTIBOOT_LOADER_COMMON

#include <stdint.h>
#include <stdnoreturn.h>

namespace loader::common
{
    void abort();
    void debug_printf(const char* str);
    void init();
} // namespace common


#endif