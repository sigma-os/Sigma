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
    // End of Exceptions
    C_LINKAGE void isr32();
    C_LINKAGE void isr33();
    C_LINKAGE void isr34();
    C_LINKAGE void isr35();
    C_LINKAGE void isr36();
    C_LINKAGE void isr37();
    C_LINKAGE void isr38();
    C_LINKAGE void isr39();
    C_LINKAGE void isr40();
    C_LINKAGE void isr41();
    C_LINKAGE void isr42();
    C_LINKAGE void isr43();
    C_LINKAGE void isr44();
    C_LINKAGE void isr45();
    C_LINKAGE void isr46();
    C_LINKAGE void isr47();
    C_LINKAGE void isr48();
    C_LINKAGE void isr49();
    C_LINKAGE void isr50();
    C_LINKAGE void isr51();
    C_LINKAGE void isr52();
    C_LINKAGE void isr53();
    C_LINKAGE void isr54();
    C_LINKAGE void isr55();
    C_LINKAGE void isr56();
    C_LINKAGE void isr57();
    C_LINKAGE void isr58();
    C_LINKAGE void isr59();
    C_LINKAGE void isr60();
    C_LINKAGE void isr61();
    C_LINKAGE void isr62();
    C_LINKAGE void isr63();
    C_LINKAGE void isr64();
    C_LINKAGE void isr65();
    C_LINKAGE void isr66();
    C_LINKAGE void isr67();
    C_LINKAGE void isr68();
    C_LINKAGE void isr69();
    C_LINKAGE void isr70();
    C_LINKAGE void isr71();
    C_LINKAGE void isr72();
    C_LINKAGE void isr73();
    C_LINKAGE void isr74();
    C_LINKAGE void isr75();
    C_LINKAGE void isr76();
    C_LINKAGE void isr76();
    C_LINKAGE void isr77();
    C_LINKAGE void isr78();
    C_LINKAGE void isr79();
    C_LINKAGE void isr80();
    C_LINKAGE void isr81();
    C_LINKAGE void isr82();
    C_LINKAGE void isr83();
    C_LINKAGE void isr84();
    C_LINKAGE void isr85();
    C_LINKAGE void isr86();
    C_LINKAGE void isr87();
    C_LINKAGE void isr88();
    C_LINKAGE void isr89();
    C_LINKAGE void isr90();
    C_LINKAGE void isr91();
    C_LINKAGE void isr92();
    C_LINKAGE void isr93();
    C_LINKAGE void isr94();
    C_LINKAGE void isr95();
    C_LINKAGE void isr96();
    C_LINKAGE void isr97();
    C_LINKAGE void isr98();
    C_LINKAGE void isr99();
    C_LINKAGE void isr100();
    C_LINKAGE void isr101();
    C_LINKAGE void isr102();
    C_LINKAGE void isr103();
    C_LINKAGE void isr104();
    C_LINKAGE void isr105();
    C_LINKAGE void isr106();
    C_LINKAGE void isr107();
    C_LINKAGE void isr108();
    C_LINKAGE void isr109();
    C_LINKAGE void isr110();
    C_LINKAGE void isr111();
    C_LINKAGE void isr112();
    C_LINKAGE void isr113();
    C_LINKAGE void isr114();
    C_LINKAGE void isr115();
    C_LINKAGE void isr116();
    C_LINKAGE void isr117();
    C_LINKAGE void isr118();
    C_LINKAGE void isr119();
    C_LINKAGE void isr120();
    C_LINKAGE void isr121();
    C_LINKAGE void isr122();
    C_LINKAGE void isr123();
    C_LINKAGE void isr124();
    C_LINKAGE void isr125();
    C_LINKAGE void isr126();
    C_LINKAGE void isr127();
    C_LINKAGE void isr128();
    C_LINKAGE void isr129();
    C_LINKAGE void isr130();
    C_LINKAGE void isr131();
    C_LINKAGE void isr132();
    C_LINKAGE void isr133();
    C_LINKAGE void isr134();
    C_LINKAGE void isr135();
    C_LINKAGE void isr136();
    C_LINKAGE void isr137();
    C_LINKAGE void isr138();
    C_LINKAGE void isr139();
    C_LINKAGE void isr140();
    C_LINKAGE void isr141();
    C_LINKAGE void isr142();
    C_LINKAGE void isr143();
    C_LINKAGE void isr144();
    C_LINKAGE void isr145();
    C_LINKAGE void isr146();
    C_LINKAGE void isr147();
    C_LINKAGE void isr148();
    C_LINKAGE void isr149();
    C_LINKAGE void isr150();
    C_LINKAGE void isr151();
    C_LINKAGE void isr152();
    C_LINKAGE void isr153();
    C_LINKAGE void isr154();
    C_LINKAGE void isr155();
    C_LINKAGE void isr156();
    C_LINKAGE void isr157();
    C_LINKAGE void isr158();
    C_LINKAGE void isr159();
    C_LINKAGE void isr160();
    C_LINKAGE void isr161();
    C_LINKAGE void isr162();
    C_LINKAGE void isr163();
    C_LINKAGE void isr164();
    C_LINKAGE void isr165();
    C_LINKAGE void isr166();
    C_LINKAGE void isr167();
    C_LINKAGE void isr168();
    C_LINKAGE void isr169();
    C_LINKAGE void isr170();
    C_LINKAGE void isr171();
    C_LINKAGE void isr172();
    C_LINKAGE void isr173();
    C_LINKAGE void isr174();
    C_LINKAGE void isr175();
    C_LINKAGE void isr176();
    C_LINKAGE void isr177();
    C_LINKAGE void isr178();
    C_LINKAGE void isr179();
    C_LINKAGE void isr180();
    C_LINKAGE void isr181();
    C_LINKAGE void isr182();
    C_LINKAGE void isr183();
    C_LINKAGE void isr184();
    C_LINKAGE void isr185();
    C_LINKAGE void isr186();
    C_LINKAGE void isr187();
    C_LINKAGE void isr188();
    C_LINKAGE void isr189();
    C_LINKAGE void isr190();
    C_LINKAGE void isr191();
    C_LINKAGE void isr192();
    C_LINKAGE void isr193();
    C_LINKAGE void isr194();
    C_LINKAGE void isr195();
    C_LINKAGE void isr196();
    C_LINKAGE void isr197();
    C_LINKAGE void isr198();
    C_LINKAGE void isr199();
    C_LINKAGE void isr200();
    C_LINKAGE void isr201();
    C_LINKAGE void isr202();
    C_LINKAGE void isr203();
    C_LINKAGE void isr204();
    C_LINKAGE void isr205();
    C_LINKAGE void isr206();
    C_LINKAGE void isr207();
    C_LINKAGE void isr208();
    C_LINKAGE void isr209();
    C_LINKAGE void isr210();
    C_LINKAGE void isr211();
    C_LINKAGE void isr212();
    C_LINKAGE void isr213();
    C_LINKAGE void isr214();
    C_LINKAGE void isr215();
    C_LINKAGE void isr216();
    C_LINKAGE void isr217();
    C_LINKAGE void isr218();
    C_LINKAGE void isr219();
    C_LINKAGE void isr220();
    C_LINKAGE void isr221();
    C_LINKAGE void isr222();
    C_LINKAGE void isr223();
    C_LINKAGE void isr224();
    C_LINKAGE void isr225();
    C_LINKAGE void isr226();
    C_LINKAGE void isr227();
    C_LINKAGE void isr228();
    C_LINKAGE void isr229();
    C_LINKAGE void isr230();
    C_LINKAGE void isr231();
    C_LINKAGE void isr232();
    C_LINKAGE void isr233();
    C_LINKAGE void isr234();
    C_LINKAGE void isr235();
    C_LINKAGE void isr236();
    C_LINKAGE void isr237();
    C_LINKAGE void isr238();
    C_LINKAGE void isr239();
    C_LINKAGE void isr240();
    C_LINKAGE void isr241();
    C_LINKAGE void isr242();
    C_LINKAGE void isr243();
    C_LINKAGE void isr244();
    C_LINKAGE void isr245();
    C_LINKAGE void isr246();
    C_LINKAGE void isr247();
    C_LINKAGE void isr248();
    C_LINKAGE void isr249();
    C_LINKAGE void isr250();
    C_LINKAGE void isr251();
    C_LINKAGE void isr252();
    C_LINKAGE void isr253();
    C_LINKAGE void isr254();
    C_LINKAGE void isr255();


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

            // end of exceptions
            this->table.set_entry(31, (void*)&isr31, x86_64::gdt::code_selector);
            this->table.set_entry(32, (void*)&isr32, x86_64::gdt::code_selector);
            this->table.set_entry(33, (void*)&isr33, x86_64::gdt::code_selector);
            this->table.set_entry(34, (void*)&isr34, x86_64::gdt::code_selector);
            this->table.set_entry(35, (void*)&isr35, x86_64::gdt::code_selector);
            this->table.set_entry(36, (void*)&isr36, x86_64::gdt::code_selector);
            this->table.set_entry(37, (void*)&isr37, x86_64::gdt::code_selector);
            this->table.set_entry(38, (void*)&isr38, x86_64::gdt::code_selector);
            this->table.set_entry(39, (void*)&isr39, x86_64::gdt::code_selector);
            this->table.set_entry(40, (void*)&isr40, x86_64::gdt::code_selector);
            this->table.set_entry(41, (void*)&isr41, x86_64::gdt::code_selector);
            this->table.set_entry(42, (void*)&isr42, x86_64::gdt::code_selector);
            this->table.set_entry(43, (void*)&isr43, x86_64::gdt::code_selector);
            this->table.set_entry(44, (void*)&isr44, x86_64::gdt::code_selector);
            this->table.set_entry(45, (void*)&isr45, x86_64::gdt::code_selector);
            this->table.set_entry(46, (void*)&isr46, x86_64::gdt::code_selector);
            this->table.set_entry(47, (void*)&isr47, x86_64::gdt::code_selector);
            this->table.set_entry(48, (void*)&isr48, x86_64::gdt::code_selector);
            this->table.set_entry(49, (void*)&isr49, x86_64::gdt::code_selector);
            this->table.set_entry(50, (void*)&isr50, x86_64::gdt::code_selector);
            this->table.set_entry(51, (void*)&isr51, x86_64::gdt::code_selector);
            this->table.set_entry(52, (void*)&isr52, x86_64::gdt::code_selector);
            this->table.set_entry(53, (void*)&isr53, x86_64::gdt::code_selector);
            this->table.set_entry(54, (void*)&isr54, x86_64::gdt::code_selector);
            this->table.set_entry(55, (void*)&isr55, x86_64::gdt::code_selector);
            this->table.set_entry(56, (void*)&isr56, x86_64::gdt::code_selector);
            this->table.set_entry(57, (void*)&isr57, x86_64::gdt::code_selector);
            this->table.set_entry(58, (void*)&isr58, x86_64::gdt::code_selector);
            this->table.set_entry(59, (void*)&isr59, x86_64::gdt::code_selector);
            this->table.set_entry(60, (void*)&isr60, x86_64::gdt::code_selector);
            this->table.set_entry(61, (void*)&isr61, x86_64::gdt::code_selector);
            this->table.set_entry(62, (void*)&isr62, x86_64::gdt::code_selector);
            this->table.set_entry(63, (void*)&isr63, x86_64::gdt::code_selector);
            this->table.set_entry(64, (void*)&isr64, x86_64::gdt::code_selector);
            this->table.set_entry(65, (void*)&isr65, x86_64::gdt::code_selector);
            this->table.set_entry(66, (void*)&isr66, x86_64::gdt::code_selector);
            this->table.set_entry(67, (void*)&isr67, x86_64::gdt::code_selector);
            this->table.set_entry(68, (void*)&isr68, x86_64::gdt::code_selector);
            this->table.set_entry(69, (void*)&isr69, x86_64::gdt::code_selector);
            this->table.set_entry(70, (void*)&isr70, x86_64::gdt::code_selector);
            this->table.set_entry(71, (void*)&isr71, x86_64::gdt::code_selector);
            this->table.set_entry(72, (void*)&isr72, x86_64::gdt::code_selector);
            this->table.set_entry(73, (void*)&isr73, x86_64::gdt::code_selector);
            this->table.set_entry(74, (void*)&isr74, x86_64::gdt::code_selector);
            this->table.set_entry(75, (void*)&isr75, x86_64::gdt::code_selector);
            this->table.set_entry(76, (void*)&isr76, x86_64::gdt::code_selector);
            this->table.set_entry(77, (void*)&isr77, x86_64::gdt::code_selector);
            this->table.set_entry(78, (void*)&isr78, x86_64::gdt::code_selector);
            this->table.set_entry(79, (void*)&isr79, x86_64::gdt::code_selector);
            this->table.set_entry(80, (void*)&isr80, x86_64::gdt::code_selector);
            this->table.set_entry(81, (void*)&isr81, x86_64::gdt::code_selector);
            this->table.set_entry(82, (void*)&isr82, x86_64::gdt::code_selector);
            this->table.set_entry(83, (void*)&isr83, x86_64::gdt::code_selector);
            this->table.set_entry(84, (void*)&isr84, x86_64::gdt::code_selector);
            this->table.set_entry(85, (void*)&isr85, x86_64::gdt::code_selector);
            this->table.set_entry(86, (void*)&isr86, x86_64::gdt::code_selector);
            this->table.set_entry(87, (void*)&isr87, x86_64::gdt::code_selector);
            this->table.set_entry(88, (void*)&isr88, x86_64::gdt::code_selector);
            this->table.set_entry(89, (void*)&isr89, x86_64::gdt::code_selector);
            this->table.set_entry(90, (void*)&isr90, x86_64::gdt::code_selector);
            this->table.set_entry(91, (void*)&isr91, x86_64::gdt::code_selector);
            this->table.set_entry(92, (void*)&isr92, x86_64::gdt::code_selector);
            this->table.set_entry(93, (void*)&isr93, x86_64::gdt::code_selector);
            this->table.set_entry(94, (void*)&isr94, x86_64::gdt::code_selector);
            this->table.set_entry(95, (void*)&isr95, x86_64::gdt::code_selector);
            this->table.set_entry(96, (void*)&isr96, x86_64::gdt::code_selector);
            this->table.set_entry(97, (void*)&isr97, x86_64::gdt::code_selector);
            this->table.set_entry(98, (void*)&isr98, x86_64::gdt::code_selector);
            this->table.set_entry(99, (void*)&isr99, x86_64::gdt::code_selector);
            this->table.set_entry(100, (void*)&isr100, x86_64::gdt::code_selector);
            this->table.set_entry(101, (void*)&isr101, x86_64::gdt::code_selector);
            this->table.set_entry(102, (void*)&isr102, x86_64::gdt::code_selector);
            this->table.set_entry(103, (void*)&isr103, x86_64::gdt::code_selector);
            this->table.set_entry(104, (void*)&isr104, x86_64::gdt::code_selector);
            this->table.set_entry(105, (void*)&isr105, x86_64::gdt::code_selector);
            this->table.set_entry(106, (void*)&isr106, x86_64::gdt::code_selector);
            this->table.set_entry(107, (void*)&isr107, x86_64::gdt::code_selector);
            this->table.set_entry(108, (void*)&isr108, x86_64::gdt::code_selector);
            this->table.set_entry(109, (void*)&isr109, x86_64::gdt::code_selector);
            this->table.set_entry(110, (void*)&isr110, x86_64::gdt::code_selector);
            this->table.set_entry(111, (void*)&isr111, x86_64::gdt::code_selector);
            this->table.set_entry(112, (void*)&isr112, x86_64::gdt::code_selector);
            this->table.set_entry(113, (void*)&isr113, x86_64::gdt::code_selector);
            this->table.set_entry(114, (void*)&isr114, x86_64::gdt::code_selector);
            this->table.set_entry(115, (void*)&isr115, x86_64::gdt::code_selector);
            this->table.set_entry(116, (void*)&isr116, x86_64::gdt::code_selector);
            this->table.set_entry(117, (void*)&isr117, x86_64::gdt::code_selector);
            this->table.set_entry(118, (void*)&isr118, x86_64::gdt::code_selector);
            this->table.set_entry(119, (void*)&isr119, x86_64::gdt::code_selector);
            this->table.set_entry(120, (void*)&isr120, x86_64::gdt::code_selector);
            this->table.set_entry(121, (void*)&isr121, x86_64::gdt::code_selector);
            this->table.set_entry(122, (void*)&isr122, x86_64::gdt::code_selector);
            this->table.set_entry(123, (void*)&isr123, x86_64::gdt::code_selector);
            this->table.set_entry(124, (void*)&isr124, x86_64::gdt::code_selector);
            this->table.set_entry(125, (void*)&isr125, x86_64::gdt::code_selector);
            this->table.set_entry(126, (void*)&isr126, x86_64::gdt::code_selector);
            this->table.set_entry(127, (void*)&isr127, x86_64::gdt::code_selector);
            this->table.set_entry(128, (void*)&isr128, x86_64::gdt::code_selector);
            this->table.set_entry(129, (void*)&isr129, x86_64::gdt::code_selector);
            this->table.set_entry(130, (void*)&isr130, x86_64::gdt::code_selector);
            this->table.set_entry(131, (void*)&isr131, x86_64::gdt::code_selector);
            this->table.set_entry(132, (void*)&isr132, x86_64::gdt::code_selector);
            this->table.set_entry(133, (void*)&isr133, x86_64::gdt::code_selector);
            this->table.set_entry(134, (void*)&isr134, x86_64::gdt::code_selector);
            this->table.set_entry(135, (void*)&isr135, x86_64::gdt::code_selector);
            this->table.set_entry(136, (void*)&isr136, x86_64::gdt::code_selector);
            this->table.set_entry(137, (void*)&isr137, x86_64::gdt::code_selector);
            this->table.set_entry(138, (void*)&isr138, x86_64::gdt::code_selector);
            this->table.set_entry(139, (void*)&isr139, x86_64::gdt::code_selector);
            this->table.set_entry(140, (void*)&isr140, x86_64::gdt::code_selector);
            this->table.set_entry(141, (void*)&isr141, x86_64::gdt::code_selector);
            this->table.set_entry(142, (void*)&isr142, x86_64::gdt::code_selector);
            this->table.set_entry(143, (void*)&isr143, x86_64::gdt::code_selector);
            this->table.set_entry(144, (void*)&isr144, x86_64::gdt::code_selector);
            this->table.set_entry(145, (void*)&isr145, x86_64::gdt::code_selector);
            this->table.set_entry(146, (void*)&isr146, x86_64::gdt::code_selector);
            this->table.set_entry(147, (void*)&isr147, x86_64::gdt::code_selector);
            this->table.set_entry(148, (void*)&isr148, x86_64::gdt::code_selector);
            this->table.set_entry(149, (void*)&isr149, x86_64::gdt::code_selector);
            this->table.set_entry(150, (void*)&isr150, x86_64::gdt::code_selector);
            this->table.set_entry(151, (void*)&isr151, x86_64::gdt::code_selector);
            this->table.set_entry(152, (void*)&isr152, x86_64::gdt::code_selector);
            this->table.set_entry(153, (void*)&isr153, x86_64::gdt::code_selector);
            this->table.set_entry(154, (void*)&isr154, x86_64::gdt::code_selector);
            this->table.set_entry(155, (void*)&isr155, x86_64::gdt::code_selector);
            this->table.set_entry(156, (void*)&isr156, x86_64::gdt::code_selector);
            this->table.set_entry(157, (void*)&isr157, x86_64::gdt::code_selector);
            this->table.set_entry(158, (void*)&isr158, x86_64::gdt::code_selector);
            this->table.set_entry(159, (void*)&isr159, x86_64::gdt::code_selector);
            this->table.set_entry(160, (void*)&isr160, x86_64::gdt::code_selector);
            this->table.set_entry(161, (void*)&isr161, x86_64::gdt::code_selector);
            this->table.set_entry(162, (void*)&isr162, x86_64::gdt::code_selector);
            this->table.set_entry(163, (void*)&isr163, x86_64::gdt::code_selector);
            this->table.set_entry(164, (void*)&isr164, x86_64::gdt::code_selector);
            this->table.set_entry(165, (void*)&isr165, x86_64::gdt::code_selector);
            this->table.set_entry(166, (void*)&isr166, x86_64::gdt::code_selector);
            this->table.set_entry(167, (void*)&isr167, x86_64::gdt::code_selector);
            this->table.set_entry(168, (void*)&isr168, x86_64::gdt::code_selector);
            this->table.set_entry(169, (void*)&isr169, x86_64::gdt::code_selector);
            this->table.set_entry(170, (void*)&isr170, x86_64::gdt::code_selector);
            this->table.set_entry(171, (void*)&isr171, x86_64::gdt::code_selector);
            this->table.set_entry(172, (void*)&isr172, x86_64::gdt::code_selector);
            this->table.set_entry(173, (void*)&isr173, x86_64::gdt::code_selector);
            this->table.set_entry(174, (void*)&isr174, x86_64::gdt::code_selector);
            this->table.set_entry(175, (void*)&isr175, x86_64::gdt::code_selector);
            this->table.set_entry(176, (void*)&isr176, x86_64::gdt::code_selector);
            this->table.set_entry(177, (void*)&isr177, x86_64::gdt::code_selector);
            this->table.set_entry(178, (void*)&isr178, x86_64::gdt::code_selector);
            this->table.set_entry(179, (void*)&isr179, x86_64::gdt::code_selector);
            this->table.set_entry(180, (void*)&isr180, x86_64::gdt::code_selector);
            this->table.set_entry(181, (void*)&isr181, x86_64::gdt::code_selector);
            this->table.set_entry(182, (void*)&isr182, x86_64::gdt::code_selector);
            this->table.set_entry(183, (void*)&isr183, x86_64::gdt::code_selector);
            this->table.set_entry(184, (void*)&isr184, x86_64::gdt::code_selector);
            this->table.set_entry(185, (void*)&isr185, x86_64::gdt::code_selector);
            this->table.set_entry(186, (void*)&isr186, x86_64::gdt::code_selector);
            this->table.set_entry(187, (void*)&isr187, x86_64::gdt::code_selector);
            this->table.set_entry(188, (void*)&isr188, x86_64::gdt::code_selector);
            this->table.set_entry(189, (void*)&isr189, x86_64::gdt::code_selector);
            this->table.set_entry(190, (void*)&isr190, x86_64::gdt::code_selector);
            this->table.set_entry(191, (void*)&isr191, x86_64::gdt::code_selector);
            this->table.set_entry(192, (void*)&isr192, x86_64::gdt::code_selector);
            this->table.set_entry(193, (void*)&isr193, x86_64::gdt::code_selector);
            this->table.set_entry(194, (void*)&isr194, x86_64::gdt::code_selector);
            this->table.set_entry(195, (void*)&isr195, x86_64::gdt::code_selector);
            this->table.set_entry(196, (void*)&isr196, x86_64::gdt::code_selector);
            this->table.set_entry(197, (void*)&isr197, x86_64::gdt::code_selector);
            this->table.set_entry(198, (void*)&isr198, x86_64::gdt::code_selector);
            this->table.set_entry(199, (void*)&isr199, x86_64::gdt::code_selector);
            this->table.set_entry(200, (void*)&isr200, x86_64::gdt::code_selector);
            this->table.set_entry(201, (void*)&isr201, x86_64::gdt::code_selector);
            this->table.set_entry(202, (void*)&isr202, x86_64::gdt::code_selector);
            this->table.set_entry(203, (void*)&isr203, x86_64::gdt::code_selector);
            this->table.set_entry(204, (void*)&isr204, x86_64::gdt::code_selector);
            this->table.set_entry(205, (void*)&isr205, x86_64::gdt::code_selector);
            this->table.set_entry(206, (void*)&isr206, x86_64::gdt::code_selector);
            this->table.set_entry(207, (void*)&isr207, x86_64::gdt::code_selector);
            this->table.set_entry(208, (void*)&isr208, x86_64::gdt::code_selector);
            this->table.set_entry(209, (void*)&isr209, x86_64::gdt::code_selector);
            this->table.set_entry(210, (void*)&isr210, x86_64::gdt::code_selector);
            this->table.set_entry(211, (void*)&isr211, x86_64::gdt::code_selector);
            this->table.set_entry(212, (void*)&isr212, x86_64::gdt::code_selector);
            this->table.set_entry(213, (void*)&isr213, x86_64::gdt::code_selector);
            this->table.set_entry(214, (void*)&isr214, x86_64::gdt::code_selector);
            this->table.set_entry(215, (void*)&isr215, x86_64::gdt::code_selector);
            this->table.set_entry(216, (void*)&isr216, x86_64::gdt::code_selector);
            this->table.set_entry(217, (void*)&isr217, x86_64::gdt::code_selector);
            this->table.set_entry(218, (void*)&isr218, x86_64::gdt::code_selector);
            this->table.set_entry(219, (void*)&isr219, x86_64::gdt::code_selector);
            this->table.set_entry(220, (void*)&isr220, x86_64::gdt::code_selector);
            this->table.set_entry(221, (void*)&isr221, x86_64::gdt::code_selector);
            this->table.set_entry(222, (void*)&isr222, x86_64::gdt::code_selector);
            this->table.set_entry(223, (void*)&isr223, x86_64::gdt::code_selector);
            this->table.set_entry(224, (void*)&isr224, x86_64::gdt::code_selector);
            this->table.set_entry(225, (void*)&isr225, x86_64::gdt::code_selector);
            this->table.set_entry(226, (void*)&isr226, x86_64::gdt::code_selector);
            this->table.set_entry(227, (void*)&isr227, x86_64::gdt::code_selector);
            this->table.set_entry(228, (void*)&isr228, x86_64::gdt::code_selector);
            this->table.set_entry(229, (void*)&isr229, x86_64::gdt::code_selector);
            this->table.set_entry(230, (void*)&isr230, x86_64::gdt::code_selector);
            this->table.set_entry(231, (void*)&isr231, x86_64::gdt::code_selector);
            this->table.set_entry(232, (void*)&isr232, x86_64::gdt::code_selector);
            this->table.set_entry(233, (void*)&isr233, x86_64::gdt::code_selector);
            this->table.set_entry(234, (void*)&isr234, x86_64::gdt::code_selector);
            this->table.set_entry(235, (void*)&isr235, x86_64::gdt::code_selector);
            this->table.set_entry(236, (void*)&isr236, x86_64::gdt::code_selector);
            this->table.set_entry(237, (void*)&isr237, x86_64::gdt::code_selector);
            this->table.set_entry(238, (void*)&isr238, x86_64::gdt::code_selector);
            this->table.set_entry(239, (void*)&isr239, x86_64::gdt::code_selector);
            this->table.set_entry(240, (void*)&isr240, x86_64::gdt::code_selector);
            this->table.set_entry(241, (void*)&isr241, x86_64::gdt::code_selector);
            this->table.set_entry(242, (void*)&isr242, x86_64::gdt::code_selector);
            this->table.set_entry(243, (void*)&isr243, x86_64::gdt::code_selector);
            this->table.set_entry(244, (void*)&isr244, x86_64::gdt::code_selector);
            this->table.set_entry(245, (void*)&isr245, x86_64::gdt::code_selector);
            this->table.set_entry(246, (void*)&isr246, x86_64::gdt::code_selector);
            this->table.set_entry(247, (void*)&isr247, x86_64::gdt::code_selector);
            this->table.set_entry(248, (void*)&isr248, x86_64::gdt::code_selector);
            this->table.set_entry(249, (void*)&isr249, x86_64::gdt::code_selector);
            this->table.set_entry(250, (void*)&isr250, x86_64::gdt::code_selector);
            this->table.set_entry(251, (void*)&isr251, x86_64::gdt::code_selector);
            this->table.set_entry(252, (void*)&isr252, x86_64::gdt::code_selector);
            this->table.set_entry(253, (void*)&isr253, x86_64::gdt::code_selector);
            this->table.set_entry(254, (void*)&isr254, x86_64::gdt::code_selector);
            this->table.set_entry(255, (void*)&isr255, x86_64::gdt::code_selector);

            this->pointer.update_idtr();
        }
    };


    void register_interrupt_handler(uint16_t n, x86_64::idt::idt_function f);

} // x86_64::idt


#endif // !SIGMA_KERNEL_IDT
