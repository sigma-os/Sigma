#ifndef SIGMA_KERNEL_PIC
#define SIGMA_KERNEL_PIC

#include <Sigma/common.h>
#include <Sigma/bitops.h>
#include <Sigma/arch/x86_64/io.h>

#include <Sigma/interfaces/interrupt_source.h>

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

    constexpr uint8_t bios_default_pic1_vector = 0x8;
    constexpr uint8_t bios_default_pic2_vector = 0x70;

    class pic : virtual public IInterruptController{
        public:
            explicit pic(): pic1_int_base(bios_default_pic1_vector), pic2_int_base(bios_default_pic2_vector){ }

            void init(){
                this->enable();
            }

            void deinit(){
                this->disable();
            }

            void set_base_vector(uint8_t base){
                this->remap(base, (base + 8));
            }

            void remap(uint8_t pic1_base, uint8_t pic2_base);

            void disable() {
                x86_64::io::outb(x86_64::pic::pic1_data_port, 0xFF); // Mask all interrupts as to "disable" it
                x86_64::io::outb(x86_64::pic::pic2_data_port, 0xFF);
            }

            void enable() {
                x86_64::io::outb(x86_64::pic::pic1_data_port, 0x0); // Unmask all interrupts to enable it
                x86_64::io::outb(x86_64::pic::pic2_data_port, 0x0);
            }

            void send_eoi();


        private:
            uint16_t get_irq_reg(uint8_t ocw3);

            uint16_t get_isr(){
                return this->get_irq_reg(x86_64::pic::read_isr);
            }


            uint8_t pic1_int_base;
            uint8_t pic2_int_base;
    };
} // x86_64::pic


#endif