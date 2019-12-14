#pragma once

#include <stddef.h>
#include <klibc/stdlib.h>
#include <Sigma/arch/x86_64/misc/spinlock.h>

namespace __cxxabiv1
{
    __extension__ typedef uint16_t __guard;

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