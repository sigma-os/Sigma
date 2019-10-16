#include <klibc/stdio.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <limits.h>
#include <stdarg.h>
#include <klibc/string.h>
#include <klibc/stdlib.h>

#include <Sigma/arch/x86_64/misc/spinlock.h>


x86_64::vga::writer main_writer = x86_64::vga::writer();
x86_64::serial::writer debug_writer = x86_64::serial::writer(x86_64::serial::com1_base);

x86_64::spinlock::mutex printf_lock = x86_64::spinlock::mutex();

static bool print(const char* data, size_t length){

    //for(uint64_t i = 0; i < length; i++) main_writer.print_char(bytes[i]);
    main_writer.nprint(data, length);

    return true;
}

int printf(const char* format, ...){
    std::lock_guard guard{printf_lock};

    va_list parameters;
    va_start(parameters, format);

    uint64_t written = 0;

    while(*format != '\0'){
        size_t maxrem = INT_MAX - written;

        if(format[0] != '%' || format[1] == '%'){
            if(format[0] == '%') format++;

            size_t amount = 1;
            while(format[amount] && format[amount] != '%') amount++;

            if(maxrem < amount){
                va_end(parameters);
                return -1;
            } 

            if(!print(format, amount)){
                va_end(parameters);
                return -1;
            } 

            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if(*format == 'c'){
            format++;
            char c = (char)va_arg(parameters, int);
            if(!maxrem){
                va_end(parameters);
                return -1;
            } 

            if(!print(&c, sizeof(c))){
                va_end(parameters);
                return -1;
            } 

            written++;
        } else if(*format == 's'){
            format++;
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else if(*format == 'i' || *format == 'd'){
            format++;
            int item = va_arg(parameters, int);

            char str[32] = "";

            itoa(item, str, 10);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else if(*format == 'x'){
            format++;
            uint64_t item = va_arg(parameters, uint64_t);

            char str[32] = "";
            //str[0] = '0';
            //str[1] = 'x';

            htoa(item, str);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else {
            format = format_begun_at;
            size_t len = strlen(format);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!print(format, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
}

int putchar(int c){
    main_writer.print_char(c);
    return c;
}

int puts(const char* s){
    return printf("%s\n", s);
}




// DEBUG LOG

#ifdef DEBUG

static bool debug_print(const char* data, size_t length){

    //for(uint64_t i = 0; i < length; i++) debug_writer.print_char(bytes[i]);
    debug_writer.nprint(data, length);

    return true;
}

static x86_64::spinlock::mutex debug_mutex{};

int debug_printf(const char* format, ...){
    std::lock_guard guard{debug_mutex};
    va_list parameters;
    va_start(parameters, format);

    uint64_t written = 0;

    while(*format != '\0'){
        size_t maxrem = INT_MAX - written;

        if(format[0] != '%' || format[1] == '%'){
            if(format[0] == '%') format++;

            size_t amount = 1;
            while(format[amount] && format[amount] != '%') amount++;

            if(maxrem < amount){
                va_end(parameters);
                return -1;
            } 

            if(!debug_print(format, amount)){
                va_end(parameters);
                return -1;
            } 

            format += amount;
            written += amount;
            continue;
        }

        const char* format_begun_at = format++;

        if(*format == 'c'){
            format++;
            char c = (char)va_arg(parameters, int);
            if(!maxrem){
                va_end(parameters);
                return -1;
            } 

            if(!debug_print(&c, sizeof(c))){
                va_end(parameters);
                return -1;
            } 
            written++;
        } else if(*format == 's'){
            format++;
            const char* str = va_arg(parameters, const char*);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!debug_print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else if(*format == 'i' || *format == 'd'){
            format++;
            int item = va_arg(parameters, int);

            char str[32] = "";

            itoa(item, str, 10);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!debug_print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else if(*format == 'x'){
            format++;
            uint64_t item = va_arg(parameters, uint64_t);

            char str[32] = "";
            //str[0] = '0';
            //str[1] = 'x';

            htoa(item, str);
            size_t len = strlen(str);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!debug_print(str, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
        } else {
            format = format_begun_at;
            size_t len = strlen(format);

            if(maxrem < len){
                va_end(parameters);
                return -1;
            } 
            if(!debug_print(format, len)){
                va_end(parameters);
                return -1;
            } 

            written += len;
            format += len;
        }
    }

    va_end(parameters);
    return written;
}

#else

int debug_printf(const char* format, ...){
    (void)(format);
    return 1;
}

#endif