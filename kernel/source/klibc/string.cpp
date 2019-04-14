#include <klibc/string.h>

size_t strlen(const char* s){
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

void* memset(void* s, int c, size_t n){
    uint8_t* buf = reinterpret_cast<uint8_t*>(s);

    for(size_t i = 0; i < n; i++) buf[i] = (uint8_t)c;

    return s;
}