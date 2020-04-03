#include <klibc/string.h>

size_t strlen(const char* s){
    size_t len = 0;
    while(s[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    //while(*src)
    //   *(dest++) = *(src++);
    //*dest = 0;
    //return dest;
    return (char*)memcpy((void*)dest, (void*)src, strlen(src) + 1);
}

void* memset(void* s, int c, size_t n){
    uint8_t* buf = (uint8_t*)s;

    for(size_t i = 0; i < n; i++) buf[i] = (uint8_t)c;

    return s;
    //asm("rep stosb" : : "a"(c), "D"((uint64_t)s), "c"(n) : "memory");
    //return s;
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

void* memcpy(void* dest, const void* src, size_t n){
    uint8_t* destination = (uint8_t*)dest;
    uint8_t* source = (uint8_t*)src;

    for(size_t i = 0; i < n; i++) destination[i] = source[i];

    return dest;
    //asm("rep movsb" : : "S"((uint64_t)src), "D"((uint64_t)dest), "c"(n) : "memory");
    //return dest;
}

void* memmove(void* dstptr, const void* srcptr, size_t size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

int strcmp(const char s1[], const char s2[]) {
    for (; *s1 == *s2 && *s1; s1++, s2++)
        ;
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void* memset_aligned_4k(void* dest, int c){
    //asm("rep stosl" : : "a"(c), "D"((uint64_t)dest), "c"(1024) : "memory");
    memset(dest, c, 0x1000);
    return dest;
}

void* memcpy_aligned_4k(void* dest, void* src){
    //asm("rep movsd" : : "S"((uint64_t)src), "D"((uint64_t)dest), "c"(1024) : "memory");
    memcpy(dest, src, 0x1000);
    return dest;
}