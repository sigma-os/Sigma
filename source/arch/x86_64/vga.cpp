#include <Sigma/arch/x86_64/vga.h>

#include <klibc/stdio.h>

void x86_64::vga::write_entry(text_entry_t character, uint8_t x, uint8_t y){
    uint32_t offset = y * vga::terminal_width + x;

    volatile uint8_t* entry = (uint8_t*)(x86_64::vga::mmio + (offset * sizeof(x86_64::vga::text_entry_t)));

    *entry = character.ascii;
    *(entry + 1) = character.colour;
}

const uint64_t x86_64::vga::mmio = (0xb8000 + KERNEL_VBASE);


void x86_64::vga::writer::nprint(const char* str, size_t n){
    x86_64::spinlock::acquire(&this->mutex);

    for(size_t i = 0; i < n; i++){
        uint8_t c = str[i];

        this->print_char(c);
    }
    x86_64::spinlock::release(&this->mutex);
}

void x86_64::vga::writer::print(const char* str){
    x86_64::spinlock::acquire(&this->mutex);

    for(; *str; str++){
        uint8_t c = *str;

        this->print_char(c);
    }
    x86_64::spinlock::release(&this->mutex);
}

void x86_64::vga::writer::print_char(const char c){
    switch (c)
    {
    case '\n':
        this->x = 0;
        this->y++;

        if(y >= 25){
            this->scroll();
            y--;
        }
        this->update_hardware_cursor();
        break;

    case '\r':
        this->x = 0;
        this->update_hardware_cursor();
        break;

    case '\b':
        {
        this->x--;
        if(this->x < 0){
            this->x = 0;
            this->y--;
            if(this->y < 0) this->y = 0;
        }
        x86_64::vga::text_entry_t ent = vga::text_entry_t(' ', this->foreground, this->background);
        x86_64::vga::write_entry(ent, this->x, this->y);
        this->update_hardware_cursor();
        }
        break;

    default:
        x86_64::vga::text_entry_t entry = x86_64::vga::text_entry_t(c, this->foreground, this->background);
        x86_64::vga::write_entry(entry, x, y);
        x++;
        if(x > 80){
            x = 0;
            y++;
            if(y >= 25){
                this->scroll();
                y--;
            }
        }
        this->update_hardware_cursor();
        break;
    }
}

void x86_64::vga::writer::set_foreground(x86_64::vga::text_colour colour) {
    x86_64::spinlock::acquire(&this->mutex);
    this->foreground = colour;
    x86_64::spinlock::release(&this->mutex);
}
void x86_64::vga::writer::set_background(x86_64::vga::text_colour colour) {
    x86_64::spinlock::acquire(&this->mutex);
    this->background = colour;
    x86_64::spinlock::release(&this->mutex);
}

void x86_64::vga::writer::set_cursor(uint8_t x, uint8_t y){
    x86_64::spinlock::acquire(&this->mutex);
    this->x = x;
    this->y = y;
    if(this->x > 80){
        this->x = 0;
        this->y++;
        if(this->y >= 25){
            this->scroll();
            this->y--;
        }
    }

    this->update_hardware_cursor();
    x86_64::spinlock::release(&this->mutex);
}

void x86_64::vga::writer::scroll(){
    for(uint64_t i = 1; i < x86_64::vga::terminal_height; i++){
        uint64_t dest_offset = 2 * ((i - 1) * x86_64::vga::terminal_width);
        void* dest = reinterpret_cast<void*>(dest_offset + x86_64::vga::mmio);

        uint64_t src_offset = 2 * (i * x86_64::vga::terminal_width);
        void* src = reinterpret_cast<void*>(src_offset + x86_64::vga::mmio);
        memcpy(dest, src, x86_64::vga::terminal_width * 2);
    }
    
    for(uint64_t i = 0; i < x86_64::vga::terminal_width; i++){
        x86_64::vga::write_entry(x86_64::vga::text_entry_t(' ', x86_64::vga::text_colour::white, x86_64::vga::text_colour::black), i, x86_64::vga::terminal_height - 1);
    }

    this->x = 0;
    this->y = 25;
}

void x86_64::vga::writer::update_hardware_cursor(){
    uint16_t pos = this->y * x86_64::vga::terminal_width + this->x;

    x86_64::io::outb(vga::vga_hardware_cursor_command_port, 0x0F);
    x86_64::io::outb(vga::vga_hardware_cursor_data_port, (uint8_t)(pos & 0xFF));

    x86_64::io::outb(vga::vga_hardware_cursor_command_port, 0x0E);
    x86_64::io::outb(vga::vga_hardware_cursor_data_port, (uint8_t)((pos >> 8) & 0xFF));
}

void x86_64::vga::writer::enable_hardware_cursor(){
    x86_64::io::outb(x86_64::vga::vga_hardware_cursor_command_port, 0x0A);
    x86_64::io::outb(x86_64::vga::vga_hardware_cursor_data_port, (io::inb(vga::vga_hardware_cursor_data_port) & 0xC0) | 0);

    x86_64::io::outb(x86_64::vga::vga_hardware_cursor_command_port, 0x0B);
    x86_64::io::outb(x86_64::vga::vga_hardware_cursor_data_port, (io::inb(vga::vga_hardware_cursor_data_port) & 0xE0) | 15);
}

void x86_64::vga::writer::disable_hardware_cursor(){
    io::outb(x86_64::vga::vga_hardware_cursor_command_port, 0x0A);
    io::outb(x86_64::vga::vga_hardware_cursor_data_port, 0x20);
}