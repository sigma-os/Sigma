#include <Sigma/common.h>

static void print(char* str){
    char* mem = reinterpret_cast<char*>(0xb8000);

    while(*str){
        *mem = *str;
        *(mem + 1) = (unsigned char)0xF;
        mem += 2;
        str++;
    }
}



extern "C" void kernel_main(void* multiboot_information){
    
    char buf[] = "Welcome to kernel";

    print(buf);




    asm("cli; hlt");
}