#include <Sigma/common.h>

#include <klibc/stdio.h>



extern "C" void kernel_main(void* multiboot_information){
    
    (void)(multiboot_information);


    printf("%d, %i, %x, %s, %c Hello w\borld", 15, 20, 0xABC, "Welcome", 's');






    asm("cli; hlt");
}