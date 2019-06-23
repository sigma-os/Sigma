#include <klibc/stdlib.h>

#include <klibc/stdio.h>

#include <Sigma/mm/hmm.h>

char* itoa(int64_t value, char* str, int base){
    char* rc, *ptr, *low;

    if(base < 2 || base > 36){
        *str = '\0';
        return str;
    }

    rc = ptr = str;

    if(value < 0 && base == 10) *ptr++ = '-';

    low = ptr;

    do
    {
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + value % base];
        value /= base;
    } while (value);

    *ptr-- = '\0';

    while(low < ptr){
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

void htoa(int64_t n, char* str){
    *str++ = '0';
    *str++ = 'x';

    int8_t zeros = 0;
    int64_t tmp;
    for(int i = 60; i > 0; i -= 4){
        tmp = (n >> i) & 0xF;
        if(tmp == 0 && zeros == 0) continue;

        zeros -= 1;

        if(tmp >= 0xA) *str++ = (tmp - 0xA + 'a');
        else *str++ = (tmp + '0');
    }

    tmp = n & 0xF;

    if(tmp >= 0xA) *str++ = (tmp - 0xA + 'a');
    else *str++ = (tmp + '0');
}

void abort(void){
    printf("Kernel panic, abort\n");
    asm("cli; hlt");
    while(true);
}

void *malloc(size_t size){
    return mm::hmm::kmalloc(size);
}

void *realloc(void* ptr, size_t size){
    return mm::hmm::realloc(ptr, size);
}

void free(void* ptr){
    mm::hmm::kfree(ptr);
}