#include <Sigma/arch/x86_64/idt.h>
x86_64::idt::idt_function handlers[x86_64::idt::idt_max_entries];

const char *exeception_msg[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};


C_LINKAGE void sigma_isr_handler(x86_64::idt::idt_registers *registers){
    uint8_t n = registers->int_number & 0xFF;

    if(handlers[n] != nullptr){
        x86_64::idt::idt_function f = handlers[n];
        f(registers);
    } else {
        if(n < 32) printf("[IDT]: Received interrupt %i, %s\n", n, exeception_msg[n]);
        else printf("[IDT]: Received interrupt %i\n", n);
        asm("cli; hlt");
        return;
    }
}

void x86_64::idt::register_interrupt_handler(uint16_t n, x86_64::idt::idt_function f){
    if(handlers[n] != nullptr) printf("[IDT]: Overwriting IDT entry %i, containing %x with %x\n", n, (uint64_t)handlers[n], (uint64_t)f);
    handlers[n] = f;
}