#ifndef SIGMA_KERNEL_IDT
#define SIGMA_KERNEL_IDT

#include <Sigma/common.h>
#include <Sigma/misc/bitops.h>

#include <Sigma/arch/x86_64/gdt.h>

#include <Sigma/smp/cpu.h>

#include <klibc/stdio.h>

namespace x86_64::idt
{
    constexpr uint8_t idt_entry_options_ist_index = 0;
    constexpr uint8_t idt_entry_options_gate = 8;
    constexpr uint8_t idt_entry_options_dpl = 13;
    constexpr uint8_t idt_entry_option_present = 15;


    class PACKED_ATTRIBUTE idt_entry {
        public:
        idt_entry() = default;
        idt_entry(void* function, uint16_t selector, bool present, uint8_t ist_number);
        idt_entry(void* function, uint16_t selector, bool present, uint8_t ist_number, uint8_t dpl);
        uint16_t pointer_low;
        uint16_t gdt_sel;
        uint16_t options;
        uint16_t pointer_mid;
        uint32_t pointer_high;
        uint32_t reserved;
    };



    constexpr uint16_t idt_max_entries = 256;

    struct PACKED_ATTRIBUTE idt_table {
        public:
        idt_table(){
            for(auto& e : entries){
                e = x86_64::idt::idt_entry(nullptr, x86_64::gdt::kernel_code_selector, false, 0);
            }
        }

        void set_entry(uint16_t n, void* function, uint16_t selector, uint8_t ist_index){
            this->entries[n] = x86_64::idt::idt_entry(function, selector, true, ist_index);
        }

        void set_entry(uint16_t n, void* function, uint16_t selector, uint8_t ist_index, uint8_t dpl){
            this->entries[n] = x86_64::idt::idt_entry(function, selector, true, ist_index, dpl);
        }

        idt_entry entries[idt_max_entries];
    };

    struct PACKED_ATTRIBUTE idt_pointer {
        void store(){
            asm("lidt %0" : : "m"(*this) : "memory");
        }
        volatile uint16_t size;
        volatile uint64_t base;
    };

    struct PACKED_ATTRIBUTE idt_registers {
        uint64_t ds;
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rdi, rsi, rbp, useless, rbx, rdx, rcx, rax;
        uint64_t int_number, error_code;
        uint64_t rip, cs, rflags, rsp, ss;
    };

    constexpr uint8_t normal_ist_index = 0;
    constexpr uint8_t double_fault_ist_index = 1;
    constexpr uint8_t page_fault_ist_index = 2;
    constexpr uint8_t nmi_ist_index = 3;
    constexpr uint8_t preemption_ist_index = 4;

    uint8_t get_free_vector();

    struct handler {
        using idt_function = void (*)(idt_registers*, void*);
        uint16_t vector;
        idt_function callback;
        void* userptr = nullptr;
        bool is_irq = false, should_iret = false;
    };

    void init();
    void load();
    void register_interrupt_handler(handler h);
    void register_irq_status(uint16_t n, bool is_irq);
    void register_generic_handlers();
} // x86_64::idt


#endif // !SIGMA_KERNEL_IDT
