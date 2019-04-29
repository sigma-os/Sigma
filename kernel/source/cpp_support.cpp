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