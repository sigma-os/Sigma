#include <Sigma/arch/x86_64/vga.h>

void vga::write_entry(text_entry_t character, uint8_t x, uint8_t y){
    uint32_t offset = y * vga::terminal_width + x;

    volatile uint8_t* entry = (uint8_t*)(vga::mmio + (offset * sizeof(vga::text_entry_t)));

    *entry = character.ascii;
    *(entry + 1) = character.colour;
}

const uint64_t vga::mmio = 0xb8000;


void vga::writer::print(const char* str){
    for(; *str; str++){
        uint8_t c = *str;

        this->print_char(c);
    }
            
}

void vga::writer::print_char(const char c){
    switch (c)
    {
    case '\n':
        this->x = 0;
        this->y++;

        if(y > 25){
            //TODO: Scrolling
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
        vga::text_entry_t ent = vga::text_entry_t(' ', this->foreground, this->background);
        vga::write_entry(ent, this->x, this->y);
        this->update_hardware_cursor();
        }
        break;

    default:
        vga::text_entry_t entry = vga::text_entry_t(c, this->foreground, this->background);
        vga::write_entry(entry, x, y);
        x++;
        if(x > 80){
            x = 0;
            y++;
            if(y > 25){
                //TODO: Scrolling
                this->scroll();
                y--;
            }
        }
        this->update_hardware_cursor();
        break;
    }
}

void vga::writer::set_foreground(vga::text_colour colour) {this->foreground = colour;}
void vga::writer::set_background(vga::text_colour colour) {this->background = colour;}

void vga::writer::set_cursor(uint8_t x, uint8_t y){
    this->x = x;
    this->y = y;
    if(x > 80){
        x = 0;
        y++;
        if(y > 25){
            //TODO: Scrolling
            this->scroll();
            y--;
        }
    }

    this->update_hardware_cursor();
}

void vga::writer::scroll(){
    for(uint8_t  i = 0; i < vga::terminal_height; i++){
        for(uint8_t j = 0; j < vga::terminal_width; j++){
            uint64_t old_offset = i * vga::terminal_width + j;
            vga::text_entry_t* old_entry = (vga::text_entry_t*)(vga::mmio + (old_offset * sizeof(vga::text_entry_t))); 

            uint64_t new_offset = (i - 1) * vga::terminal_width + j;
            vga::text_entry_t* new_entry = (vga::text_entry_t*)(vga::mmio + (new_offset * sizeof(vga::text_entry_t))); 



            *new_entry = *old_entry;
        }
    }

    uint8_t old_x = this->x, old_y = this->y;
    this->set_cursor(0, vga::terminal_height - 1);

    for(uint8_t i = 0; i < vga::terminal_width; i++) this->print_char(' ');

    this->set_cursor(old_x, old_y);
}

void vga::writer::update_hardware_cursor(){
    uint16_t pos = this->y * vga::terminal_width + this->x;

    io::outb(vga::vga_hardware_cursor_command_port, 0x0F);
    io::outb(vga::vga_hardware_cursor_data_port, (uint8_t)(pos & 0xFF));

    io::outb(vga::vga_hardware_cursor_command_port, 0x0E);
    io::outb(vga::vga_hardware_cursor_data_port, (uint8_t)((pos >> 8) & 0xFF));
}

void enable_hardware_cursor(){
    io::outb(vga::vga_hardware_cursor_command_port, 0x0A);
    io::outb(vga::vga_hardware_cursor_data_port, (io::inb(vga::vga_hardware_cursor_data_port) & 0xC0) | 0);

    io::outb(vga::vga_hardware_cursor_command_port, 0x0B);
    io::outb(vga::vga_hardware_cursor_data_port, (io::inb(vga::vga_hardware_cursor_data_port) & 0xE0) | 15);
}

void disable_hardware_cursor(){
    io::outb(vga::vga_hardware_cursor_command_port, 0x0A);
    io::outb(vga::vga_hardware_cursor_data_port, 0x20);
}