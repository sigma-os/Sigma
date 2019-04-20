#ifndef SIGMA_KERNEL_PIC
#define SIGMA_KERNEL_PIC

#include <Sigma/common.h>
#include <Sigma/bitops.h>
#include <Sigma/arch/x86_64/io.h>

namespace x86_64::pic
{
    constexpr uint8_t pic1_cmd_port = 0x20;
    constexpr uint8_t pic1_data_port = 0x21;
    constexpr uint8_t pic2_cmd_port = 0xA0;
    constexpr uint8_t pic2_data_port = 0xA1;

    constexpr uint8_t eoi_cmd = 0x20;

    constexpr uint8_t read_irr = 0xa;
    constexpr uint8_t read_isr = 0xb;

    constexpr uint8_t icw1_icw4 = 0x1;
    constexpr uint8_t icw1_single = 0x2;
    constexpr uint8_t icw1_interval4 = 0x4;
    constexpr uint8_t icw1_level = 0x8;
    constexpr uint8_t icw1_init = 0x10;


    constexpr uint8_t icw4_8086 = 0x1;    
    constexpr uint8_t icw4_auto = 0x2;
    constexpr uint8_t icw4_buf_slave = 0x8;
    constexpr uint8_t icw4_buf_master = 0xC;
    constexpr uint8_t icw1_sfnm = 0x10;    

    class pic {
        public:
            pic(uint8_t base): pic1_int_base(base), pic2_int_base(base + 8){
                this->enable();

                this->remap(this->pic1_int_base, this->pic2_int_base);
            }

            void remap(uint8_t base){
                this->remap(base, (base + 8));
            }

            void remap(uint8_t pic1_base, uint8_t pic2_base){
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

                x86_64::io::outb(x86_64::pic::pic1_data_port, pic1_base); // Restore masks
                x86_64::io::outb(x86_64::pic::pic2_data_port, pic2_base);

                this->pic1_int_base = pic1_base;
                this->pic2_int_base = pic2_base;
            }

            void disable() {
                x86_64::io::outb(x86_64::pic::pic1_data_port, 0xFF); // Mask all interrupts as to "disable" it
                x86_64::io::outb(x86_64::pic::pic2_data_port, 0xFF);
            }

            void enable() {
                x86_64::io::outb(x86_64::pic::pic1_data_port, 0x0); // Unmask all interrupts to enable it
                x86_64::io::outb(x86_64::pic::pic2_data_port, 0x0);
            }

            void send_eoi(){
                uint16_t isr = this->get_isr();

                if(isr == 0) return; // PIC1 Spurious or no interrupt at all
                if(bitops<uint16_t>::bit_test(isr, 2) && ((isr >> 8) & 0xFF) == 0){
                    x86_64::io::outb(x86_64::pic::pic1_cmd_port, x86_64::pic::eoi_cmd); //PIC2 Spurious
                    return;
                }

                if(bitops<uint16_t>::bit_test(isr, 2)) x86_64::io::outb(x86_64::pic::pic2_cmd_port, x86_64::pic::eoi_cmd); //PIC2 EOI
                x86_64::io::outb(x86_64::pic::pic1_cmd_port, x86_64::pic::eoi_cmd); //PIC1 EOI
            }


        private:
            uint16_t get_irq_reg(uint8_t ocw3){
                x86_64::io::outb(x86_64::pic::pic1_cmd_port, ocw3);
                x86_64::io::outb(x86_64::pic::pic2_cmd_port, ocw3);

                return (x86_64::io::inb(pic2_data_port) << 8) | (x86_64::io::inb(pic1_data_port));
            }


            uint16_t get_isr(){
                return this->get_irq_reg(x86_64::pic::read_isr);
            }


            uint8_t pic1_int_base;
            uint8_t pic2_int_base;
    };
} // x86_64::pic


#endif