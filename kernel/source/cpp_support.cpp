#include <Sigma/cpp_support.h>

extern "C" void __cxa_pure_virtual(){
    asm("cli; hlt");
}

extern "C" int __cxxabiv1::__cxa_guard_acquire(__guard *g){
    while(g != 0) asm("pause");
    *g = 1;
    return *g;
}

extern "C" void __cxxabiv1::__cxa_guard_release(__guard *g){
    *g = 0;
}

extern "C" void __cxxabiv1::__cxa_guard_abort(__guard *g){
    (void)(g);
    asm("cli; hlt");
}