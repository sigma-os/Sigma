#ifndef SIGMA_KERNEL_IDT
#define SIGMA_KERNEL_IDT

#include <Sigma/common.h>
#include <Sigma/bitops.h>

#include <Sigma/arch/x86_64/gdt.h>

#include <Sigma/smp/cpu.h>

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
        idt_entry(void* function, uint16_t selector, bool present, uint8_t ist_number);
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
                e = x86_64::idt::idt_entry(nullptr, x86_64::gdt::code_selector, false, 0);
            }
        }

        void set_entry(uint16_t n, void* function, uint16_t selector, uint8_t ist_index){
            this->entries[n] = x86_64::idt::idt_entry(function, selector, true, ist_index);
        }

        idt_entry entries[idt_max_entries];
    } __attribute__((packed));

    struct idt_pointer {
        void update_idtr(){
            asm("lidt %0" : : "m"(*this));
        }
        volatile uint16_t size;
        volatile uint64_t base;
    } __attribute__((packed));

    struct idt_registers {
        uint64_t ds;
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, useless, rbx, rdx, rcx, rax;
        uint64_t int_number, error_code;
        uint64_t rip, cs, rflags, rsp, ss;
    } __attribute__((packed));

    typedef void (*idt_function)(idt_registers*);

    constexpr uint8_t normal_ist_index = 0;
    constexpr uint8_t double_fault_ist_index = 1;
    constexpr uint8_t page_fault_ist_index = 2;
    constexpr uint8_t nmi_ist_index = 3;

    class idt {
        public:
        idt_table table;
        idt_pointer pointer;

        
        idt(): table(idt_table()), pointer(idt_pointer()){
            this->pointer.base = (uint64_t)&table;
            this->pointer.size = (sizeof(x86_64::idt::idt_entry) * x86_64::idt::idt_max_entries) - 1;
        }

        void init();
    };


    void register_interrupt_handler(uint16_t n, x86_64::idt::idt_function f);
    void register_interrupt_handler(uint16_t n, x86_64::idt::idt_function f, bool is_irq);

    void register_irq_status(uint16_t n, bool is_irq);

    void register_generic_handlers();

    struct handler {
        handler(): callback(nullptr), is_irq(false) {}

        x86_64::idt::idt_function callback;
        bool is_irq;
    };

} // x86_64::idt


#endif // !SIGMA_KERNEL_IDT
