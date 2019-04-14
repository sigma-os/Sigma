#include <Sigma/arch/x86_64/gdt.h>

x86_64::gdt::gdt::gdt(){
    memset(this->entries, 0, (sizeof(x86_64::gdt::entry) * x86_64::gdt::max_entries));
    this->entry_index = 0;
}

void x86_64::gdt::gdt::init(){
    this->add_entry(0); // Null Entry
    this->add_entry(x86_64::gdt::entry_executable_bit | x86_64::gdt::entry_descriptor_type_bit | x86_64::gdt::entry_present_bit | x86_64::gdt::entry_64bit_code_bit); // Kernel Code

    this->pointer.pointer = (uint64_t)&this->entries;
    this->pointer.size = (sizeof(x86_64::gdt::entry) * this->entry_index) - 1;

    asm("lgdt %0" : : "m"(this->pointer));
}

uint64_t x86_64::gdt::gdt::add_entry(uint64_t flags){
    this->entries[this->entry_index++] = x86_64::gdt::entry(flags);
    return this->get_offset_by_index(this->entry_index);
}

uint64_t x86_64::gdt::gdt::get_offset_by_index(uint64_t index){
    return index * sizeof(x86_64::gdt::entry);
}