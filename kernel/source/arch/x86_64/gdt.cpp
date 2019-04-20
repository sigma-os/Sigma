#include <Sigma/arch/x86_64/gdt.h>

x86_64::gdt::gdt::gdt(){
    memset(this->entries, 0, (sizeof(x86_64::gdt::entry) * x86_64::gdt::max_entries));
    this->entry_index = 0;
}

void x86_64::gdt::gdt::init(){
    this->add_entry(0); // Null Entry
    this->add_entry(x86_64::gdt::entry_executable_bit | x86_64::gdt::entry_descriptor_type_bit | x86_64::gdt::entry_present_bit | x86_64::gdt::entry_64bit_code_bit); // Kernel Code

    this->pointer.pointer = (uint64_t)&this->entries;
    this->pointer.size = (sizeof(x86_64::gdt::entry) * x86_64::gdt::max_entries) - 1;

    this->pointer.update_gdtr();
}

uint64_t x86_64::gdt::gdt::add_entry(uint64_t flags){
    this->entries[this->entry_index++] = x86_64::gdt::entry(flags);
    return this->get_offset_by_index((this->entry_index - 1));
}

uint64_t x86_64::gdt::gdt::add_tss(x86_64::tss::table tss){
    uint64_t tss_address = reinterpret_cast<uint64_t>(&tss);
    uint32_t tss_size = (sizeof(x86_64::tss::table) - 1);
    uint32_t tss_entry_type = 0x8900;


    uint32_t ent1_low = (tss_size & 0xFFFF) | ((tss_address & 0xFFFF) << 16);
    uint32_t ent1_high = ((tss_address >> 16) & 0xFF) | tss_entry_type | (tss_size & 0x000F0000) | (tss_address & 0xFF000000);

    uint32_t ent2_low = (tss_address >> 32);

    uint64_t ent1 = (ent1_low | ((uint64_t)ent1_high << 32));
    uint64_t ent2 = ent2_low;


    uint64_t offset = this->add_entry(ent1);
    this->add_entry(ent2);
    return offset;
}

uint64_t x86_64::gdt::gdt::get_offset_by_index(uint64_t index){
    return index * sizeof(x86_64::gdt::entry);
}