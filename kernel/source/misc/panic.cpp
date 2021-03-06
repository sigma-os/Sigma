#include <Sigma/misc/panic.h>
#include <Sigma/misc/debug.h>
#include <klibc/stdio.h>
#include <klibc/stdlib.h>

NORETURN_ATTRIBUTE
void misc::panic::panic_m(const char* message, std::experimental::source_location loc){
    printf("KERNEL PANIC in file: %s in function %s on line: %d: %s\n", loc.file_name(), loc.function_name(), loc.line(), message);
    debug::trace_stack();
    abort();
}

NORETURN_ATTRIBUTE
void misc::panic::panic_m(const char* message, const char* function_override, std::experimental::source_location loc){
    printf("KERNEL PANIC in file: %s in function %s on line: %d: %s\n", loc.file_name(), function_override, loc.line(), message);
    debug::trace_stack();
    abort();
}