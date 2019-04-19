#ifndef SIGMA_KERNEL_IDT
#define SIGMA_KERNEL_IDT

#include <Sigma/common.h>
#include <Sigma/bitops.h>

#include <Sigma/arch/x86_64/gdt.h>

#include <klibc/stdio.h>

namespace x86_64::idt
{
    constexpr uint8_t idt_entry_options_ist_index = 0;
    constexpr uint8_t idt_entry_options_gate = 8;
    constexpr uint8_t idt_entry_options_dpl = 13;
    constexpr uint8_t idt_entry_option_present = 15;


    class idt_entry {
        public:
        idt_entry(){
            
        }
        idt_entry(void* function, uint16_t selector, bool present){
            uint64_t pointer = reinterpret_cast<uint64_t>(function);

            this->pointer_low = (pointer & 0xFFFF);
            this->pointer_mid = ((pointer >> 16) & 0xFFFF);
            this->pointer_high = ((pointer >> 32) & 0xFFFFFFFF);

            this->gdt_sel = selector;

            this->options = 0;
            bitops<uint16_t>::bit_set(&(this->options), 9); // Next 3 bits must be 1
            bitops<uint16_t>::bit_set(&(this->options), 10);
            bitops<uint16_t>::bit_set(&(this->options), 11);

            if(present) bitops<uint16_t>::bit_set(&(this->options), idt_entry_option_present);
        }
        uint16_t pointer_low;
        uint16_t gdt_sel;
        uint16_t options;
        uint16_t pointer_mid;
        uint32_t pointer_high;
        uint32_t reserved;
    } __attribute__((packed));



    constexpr uint16_t idt_max_entries = 256;

    struct idt_table {
        public:
        idt_table(){
            for(auto& e : entries){
                e = x86_64::idt::idt_entry(nullptr, x86_64::gdt::code_selector, false);
            }
        }

        void set_entry(uint16_t n, void* function, uint16_t selector){
            entries[n] = x86_64::idt::idt_entry(function, selector, true);
        }

        idt_entry entries[idt_max_entries];
    } __attribute__((packed));

    struct idt_pointer {
        void update_idtr(){
            asm("lidt %0" : : "m"(*this));
        }
        uint16_t size;
        uint64_t base;
    } __attribute__((packed));

    struct idt_registers {
        uint64_t ds;
        uint64_t r11, r10, r9, r8, rdi, rsi, rbp, useless, rbx, rdx, rcx, rax;
        uint64_t int_number, error_code;
        uint64_t rip, cs, rflags, rsp, ss;
    } __attribute__((packed));

    typedef void (*idt_function)(idt_registers*);

    C_LINKAGE void isr0();
    C_LINKAGE void isr1();
    C_LINKAGE void isr2();
    C_LINKAGE void isr3();
    C_LINKAGE void isr4();
    C_LINKAGE void isr5();
    C_LINKAGE void isr6();
    C_LINKAGE void isr7();
    C_LINKAGE void isr8();
    C_LINKAGE void isr9();
    C_LINKAGE void isr10();
    C_LINKAGE void isr11();
    C_LINKAGE void isr12();
    C_LINKAGE void isr13();
    C_LINKAGE void isr14();
    C_LINKAGE void isr15();
    C_LINKAGE void isr16();
    C_LINKAGE void isr17();
    C_LINKAGE void isr18();
    C_LINKAGE void isr19();
    C_LINKAGE void isr20();
    C_LINKAGE void isr21();
    C_LINKAGE void isr22();
    C_LINKAGE void isr23();
    C_LINKAGE void isr24();
    C_LINKAGE void isr25();
    C_LINKAGE void isr26();
    C_LINKAGE void isr27();
    C_LINKAGE void isr28();
    C_LINKAGE void isr29();
    C_LINKAGE void isr30();
    C_LINKAGE void isr31();


    class idt {
        private:
        idt_table table;
        idt_pointer pointer;

        public:
        idt(): table(idt_table()), pointer(idt_pointer()){
            pointer.base = (uint64_t)&table;
            pointer.size = (sizeof(x86_64::idt::idt_entry) * x86_64::idt::idt_max_entries) - 1;
        }

        void init(){
            this->table.set_entry(0, (void*)&isr0, x86_64::gdt::code_selector);
            this->table.set_entry(1, (void*)&isr1, x86_64::gdt::code_selector);
            this->table.set_entry(2, (void*)&isr2, x86_64::gdt::code_selector);
            this->table.set_entry(3, (void*)&isr3, x86_64::gdt::code_selector);
            this->table.set_entry(4, (void*)&isr4, x86_64::gdt::code_selector);
            this->table.set_entry(5, (void*)&isr5, x86_64::gdt::code_selector);
            this->table.set_entry(6, (void*)&isr6, x86_64::gdt::code_selector);
            this->table.set_entry(7, (void*)&isr7, x86_64::gdt::code_selector);
            this->table.set_entry(8, (void*)&isr8, x86_64::gdt::code_selector);
            this->table.set_entry(9, (void*)&isr9, x86_64::gdt::code_selector);
            this->table.set_entry(10, (void*)&isr10, x86_64::gdt::code_selector);
            this->table.set_entry(11, (void*)&isr11, x86_64::gdt::code_selector);
            this->table.set_entry(12, (void*)&isr12, x86_64::gdt::code_selector);
            this->table.set_entry(13, (void*)&isr13, x86_64::gdt::code_selector);
            this->table.set_entry(14, (void*)&isr14, x86_64::gdt::code_selector);
            this->table.set_entry(15, (void*)&isr15, x86_64::gdt::code_selector);
            this->table.set_entry(16, (void*)&isr16, x86_64::gdt::code_selector);
            this->table.set_entry(17, (void*)&isr17, x86_64::gdt::code_selector);
            this->table.set_entry(18, (void*)&isr18, x86_64::gdt::code_selector);
            this->table.set_entry(19, (void*)&isr19, x86_64::gdt::code_selector);
            this->table.set_entry(20, (void*)&isr20, x86_64::gdt::code_selector);
            this->table.set_entry(21, (void*)&isr21, x86_64::gdt::code_selector);
            this->table.set_entry(22, (void*)&isr22, x86_64::gdt::code_selector);
            this->table.set_entry(23, (void*)&isr23, x86_64::gdt::code_selector);
            this->table.set_entry(24, (void*)&isr24, x86_64::gdt::code_selector);
            this->table.set_entry(25, (void*)&isr25, x86_64::gdt::code_selector);
            this->table.set_entry(26, (void*)&isr26, x86_64::gdt::code_selector);
            this->table.set_entry(27, (void*)&isr27, x86_64::gdt::code_selector);
            this->table.set_entry(28, (void*)&isr28, x86_64::gdt::code_selector);
            this->table.set_entry(29, (void*)&isr29, x86_64::gdt::code_selector);
            this->table.set_entry(30, (void*)&isr30, x86_64::gdt::code_selector);
            this->table.set_entry(31, (void*)&isr31, x86_64::gdt::code_selector);


            this->pointer.update_idtr();
        }
    };


    void register_interrupt_handler(uint16_t n, x86_64::idt::idt_function f);

} // x86_64::idt


#endif // !SIGMA_KERNEL_IDT
