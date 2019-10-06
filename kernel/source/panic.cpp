#include <Sigma/panic.h>
#include <Sigma/debug.h>
#include <klibc/stdio.h>
#include <klibc/stdlib.h>

void Sigma::panic::panic_m(const char* message, const char* file, const char* func, int line){
    printf("KERNEL PANIC in file: %s in function %s on line: %d: %s\n", file, func, line, message);
    debug::trace_stack(5);
    abort();
}