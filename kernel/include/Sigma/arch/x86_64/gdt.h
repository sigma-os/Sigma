#ifndef SIGMA_ARCH_X86_64_GDT
#define SIGMA_ARCH_X86_64_GDT

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/tss.h>

#include <klibc/string.h>
#include <klibc/stdio.h>

namespace x86_64::gdt
{
    struct PACKED_ATTRIBUTE entry {
        explicit entry(uint64_t ent): ent(ent) {}
        entry() = default;

        uint64_t ent;
    };

    struct PACKED_ATTRIBUTE pointer {
        void update_gdtr(){
            asm("lgdt %0" : : "m"(*this) : "memory");
        }
        uint16_t size;
        uint64_t pointer;
    };


    
    constexpr uint64_t entry_read_write_bit = (1ULL << 41);
    constexpr uint64_t entry_conforming_bit = (1ULL << 42);
    constexpr uint64_t entry_executable_bit = (1ULL << 43);
    constexpr uint64_t entry_descriptor_type_bit = (1ULL << 44);
    constexpr uint64_t entry_privilege = (1ULL << 45);
    constexpr uint64_t entry_present_bit = (1ULL << 47);
    constexpr uint64_t entry_64bit_code_bit = (1ULL << 53);
    constexpr uint64_t entry_32bit_bit = (1ULL << 54);

    constexpr uint64_t max_entries = 10; // Maximum usable GDT entries

    constexpr uint64_t hardware_max_entries = 255;

    static_assert(max_entries < hardware_max_entries);
    
    class gdt {
        public:
            gdt();

            void init();

            uint64_t add_entry(uint64_t flags);
            uint64_t add_tss(x86_64::tss::table& tss);
            uint64_t get_offset_by_index(uint64_t index);
            x86_64::gdt::entry& get_entry_by_offset(uint64_t offset){
                return entries[offset / 8];
            }

            void update_pointer(){
                this->pointer.update_gdtr();
            }
        private:
            x86_64::gdt::entry entries[x86_64::gdt::max_entries];
            x86_64::gdt::pointer pointer;
            uint64_t entry_index;
        
    };

    constexpr uint16_t kernel_data_selector = 0x0;
    constexpr uint16_t kernel_code_selector = 0x08; // Manually update this

    constexpr uint16_t user_data_selector = 0x18;
    constexpr uint16_t user_code_selector = 0x10; // Manually update this
} // x86_64::gdt



#endif