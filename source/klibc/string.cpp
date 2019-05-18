#include <klibc/string.h>

size_t strlen(const char* s){
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

void* memset(void* s, int c, size_t n){
    uint8_t* buf = (uint8_t*)s;

    for(size_t i = 0; i < n; i++) buf[i] = (uint8_t)c;

    return s;
}

int memcmp(const void* s1, const void* s2, size_t n){
    const uint8_t* a = (const uint8_t*)s1;
    const uint8_t* b = (const uint8_t*)s2;

    for(size_t i = 0; i < n; i++){
        if(a[i] < b[i]) return -1;
        else if(b[i] < a[i]) return 1;
    }

    return 0;
}

void* memcpy(void* dest, void* src, size_t n){
    uint8_t* destination = (uint8_t*)dest;
    uint8_t* source = (uint8_t*)src;

    for(size_t i = 0; i < n; i++) destination[i] = source[i];

    return dest;
}