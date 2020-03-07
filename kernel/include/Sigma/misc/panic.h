#ifndef SIGMA_KERNEL_PANIC
#define SIGMA_KERNEL_PANIC

#include <klibcxx/experimental/source_location.hpp>
#include <Sigma/misc/compiler.h>

namespace misc::panic {
    NORETURN_ATTRIBUTE
    void panic_m(const char* message, const char* function_override, std::experimental::source_location loc);

    NORETURN_ATTRIBUTE
    void panic_m(const char* message, std::experimental::source_location loc);
}

#endif