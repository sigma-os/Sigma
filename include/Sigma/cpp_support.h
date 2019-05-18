#pragma once

#include <stddef.h>
#include <klibc/stdlib.h>

namespace __cxxabiv1
{
    __extension__ typedef int __guard __attribute__((mode(__DI__)));

    extern "C" int __cxa_guard_acquire(__guard *);
    extern "C" void __cxa_guard_release(__guard *);
    extern "C" void __cxa_guard_abort(__guard *);
}


void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* p);
void operator delete[](void* p);
void operator delete(void* p, long unsigned int size);
void operator delete[](void* p, long unsigned int size);

void *operator new(size_t, void *p);
void *operator new[](size_t, void *p);
void  operator delete  (void *, void *);
void  operator delete[](void *, void *);