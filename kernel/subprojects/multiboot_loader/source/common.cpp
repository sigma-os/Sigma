#include <loader/common.h>

void loader::common::abort(){
    debug_printf("Loader aborted\n");
    asm("cli; hlt");
    while(true);
}

static uint8_t inb(uint16_t port){
    uint8_t ret;
    asm volatile ("in %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void outb(uint16_t port, uint8_t data){
    asm volatile ("out %0, %1" : : "a"(data), "Nd"(port));
}

constexpr uint16_t com1_base = 0x3F8;
void loader::common::init(){
    
    outb(com1_base + 3, 0x03); // Configure port
    outb(com1_base + 1, 0); // Disable interrupts

    uint8_t cmd = inb(com1_base + 3);
    cmd |= (1 << 7);
    outb(com1_base + 3, cmd);
    outb(com1_base, (3 >> 8) & 0xFF);
    outb(com1_base, 3 & 0xFF);

    cmd = inb(com1_base + 3);
    cmd &= ~(1 << 7);
    outb(com1_base + 3, cmd);
}


static void print_char(const char c){
    while((inb(com1_base + 5) & 0x20) == 0);
    outb(com1_base, c);
}

void loader::common::debug_printf(const char* str){
    while(*str){
        print_char(*str);
        str++;
    }
}