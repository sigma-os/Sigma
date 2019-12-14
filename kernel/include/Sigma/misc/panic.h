#ifndef SIGMA_KERNEL_PANIC
#define SIGMA_KERNEL_PANIC

#include <klibcxx/experimental/source_location.hpp>

namespace misc::panic {
    void panic_m(const char* message, const char* function_override, std::experimental::source_location loc);
    void panic_m(const char* message, std::experimental::source_location loc);
}

#endif