#include <Sigma/arch/x86_64/drivers/pic.h>

static uint8_t pic1_int_base;
static uint8_t pic2_int_base;

static uint16_t get_irq_reg(uint8_t ocw3){
    x86_64::io::outb(x86_64::pic::pic1_cmd_port, ocw3);
    x86_64::io::outb(x86_64::pic::pic2_cmd_port, ocw3);

    return (x86_64::io::inb(x86_64::pic::pic2_data_port) << 8) | (x86_64::io::inb(x86_64::pic::pic1_data_port));
}

static uint16_t get_isr(){
    return get_irq_reg(x86_64::pic::read_isr);
}

void x86_64::pic::remap(uint8_t pic1_base, uint8_t pic2_base){
    uint8_t pic1_mask = x86_64::io::inb(x86_64::pic::pic1_data_port); // Save masks
    uint8_t pic2_mask = x86_64::io::inb(x86_64::pic::pic2_data_port);

    x86_64::io::outb(x86_64::pic::pic1_cmd_port, (x86_64::pic::icw1_init | x86_64::pic::icw1_icw4)); // Start init sequence, in cascade mode
    x86_64::io::outb(x86_64::pic::pic2_cmd_port, (x86_64::pic::icw1_init | x86_64::pic::icw1_icw4));

    x86_64::io::outb(x86_64::pic::pic1_data_port, pic1_base); // ISR vector
    x86_64::io::outb(x86_64::pic::pic2_data_port, pic2_base);

    x86_64::io::outb(x86_64::pic::pic1_data_port, 4); // Tell master that there is a slave at IRQ2
    x86_64::io::outb(x86_64::pic::pic2_data_port, 2); // Tell slave its cascade identity

    x86_64::io::outb(x86_64::pic::pic1_data_port, x86_64::pic::icw4_8086); // Set them in 8086 mode
    x86_64::io::outb(x86_64::pic::pic2_data_port,x86_64::pic::icw4_8086);

    x86_64::io::outb(x86_64::pic::pic1_data_port, pic1_mask); // Restore masks
    x86_64::io::outb(x86_64::pic::pic2_data_port, pic2_mask);

    pic1_int_base = pic1_base;
    pic2_int_base = pic2_base;
}

void x86_64::pic::send_eoi(){
    uint16_t isr = get_isr();

    if(isr == 0) return; // PIC1 Spurious or no interrupt at all
    if(bitops<uint16_t>::bit_test(isr, 2) && ((isr >> 8) & 0xFF) == 0){
        x86_64::io::outb(x86_64::pic::pic1_cmd_port, x86_64::pic::eoi_cmd); //PIC2 Spurious
        return;
    }

    if(bitops<uint16_t>::bit_test(isr, 2)) x86_64::io::outb(x86_64::pic::pic2_cmd_port, x86_64::pic::eoi_cmd); //PIC2 EOI
    x86_64::io::outb(x86_64::pic::pic1_cmd_port, x86_64::pic::eoi_cmd); //PIC1 EOI
}

void x86_64::pic::set_base_vector(uint8_t base){
    remap(base, (base + 8));
}

void x86_64::pic::disable() {
    x86_64::io::outb(x86_64::pic::pic1_data_port, 0xFF); // Mask all interrupts as to "disable" it
    x86_64::io::outb(x86_64::pic::pic2_data_port, 0xFF);
}

void x86_64::pic::enable() {
    x86_64::io::outb(x86_64::pic::pic1_data_port, 0x0); // Unmask all interrupts to enable it
    x86_64::io::outb(x86_64::pic::pic2_data_port, 0x0);
}