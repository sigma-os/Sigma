#include <Sigma/cpp_support.h>
#include <klibc/stdio.h>

extern "C" void __cxa_pure_virtual(){
    printf("__cxa_pure_virtual() called");
    asm("cli; hlt");
}

// This function is intended to register a global destructor handle to be called on exit()
// However this is a kernel, and if it tries to exit via exit() or whatever something is seriously wrong
// So we can safely ignore it
// Something something https://wiki.osdev.org/C%2B%2B#Global_objects
extern "C" int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle){
    (void)(func);
    (void)(arg);
    (void)(dso_handle);
    return 1;
}

extern "C" int __cxxabiv1::__cxa_guard_acquire(__guard *g){
    x86_64::spinlock::acquire(reinterpret_cast<x86_64::spinlock::mutex*>(g));
    return *reinterpret_cast<int*>(g);
}

extern "C" void __cxxabiv1::__cxa_guard_release(__guard *g){
    x86_64::spinlock::release(reinterpret_cast<x86_64::spinlock::mutex*>(g));
}

extern "C" void __cxxabiv1::__cxa_guard_abort(__guard *g){
    PANIC("__cxa_guard_abort was called\n");
    (void)(g);
    asm("cli; hlt");
}

void* operator new(size_t size){
    return malloc(size);
}

void* operator new[](size_t size){
    return malloc(size);
}

void operator delete(void* p){
    free(p);
}

void operator delete[](void* p){
    free(p);
}

void operator delete(void* p, long unsigned int size){
    (void)(size);
    free(p);
}

void operator delete[](void* p, long unsigned int size){
    (void)(size);
    free(p);
}