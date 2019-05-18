#include <Sigma/cpp_support.h>

extern "C" void __cxa_pure_virtual(){
    asm("cli; hlt");
}

// Register destructor function?, just don't i don't know what to do just ignore the call
// Kernel shouldn't return anyway
// Something something https://wiki.osdev.org/C%2B%2B#Global_objects
extern "C" int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle){
    (void)(func);
    (void)(arg);
    (void)(dso_handle);
    return 1;
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

inline void *operator new(size_t, void *p)     throw() { return p; }
inline void *operator new[](size_t, void *p)   throw() { return p; }
inline void  operator delete  (void *, void *) throw() { };
inline void  operator delete[](void *, void *) throw() { };

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