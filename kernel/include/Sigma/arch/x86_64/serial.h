#ifndef SIGMA_KERNEL_SERIAL
#define SIGMA_KERNEL_SERIAL

#include <Sigma/common.h>
#include <Sigma/bitops.h>

#include <Sigma/arch/x86_64/io.h>

namespace x86_64::serial
{
    constexpr uint16_t com1_base = 0x3F8;

    constexpr uint8_t interrupt_enable = 1;

    constexpr uint8_t line_control = 3;

    constexpr uint8_t modem_control = 4;
    constexpr uint8_t line_status = 5;

    constexpr uint8_t line_control_dlab = 7;


    class writer {
        public:
        explicit writer(uint16_t base): base(base), mutex(x86_64::spinlock::mutex()){
            x86_64::io::outb(this->base + x86_64::serial::line_control, 0x03); // Configure port
            x86_64::io::outb(this->base + x86_64::serial::interrupt_enable, 0); // Disable interrupts

            this->configure_baud_rate(3);
        }

        void print_char(const char c){
            while((x86_64::io::inb(this->base + x86_64::serial::line_status) & 0x20) == 0);
            x86_64::io::outb(this->base, c);
        }

        void nprint(const char* str, size_t n){
            this->mutex.acquire();

            for(size_t i = 0; i < n; i++){
                uint8_t c = str[i];

                this->print_char(c);
            }
            this->mutex.release();
        }


        private:

        void configure_baud_rate(uint16_t divisor){
            uint8_t cmd = x86_64::io::inb(this->base + x86_64::serial::line_control);
            bitops<uint8_t>::bit_set(cmd, x86_64::serial::line_control_dlab);
            x86_64::io::outb(this->base + x86_64::serial::line_control, cmd);

            x86_64::io::outb(this->base, (divisor >> 8) & 0xFF);
            x86_64::io::outb(this->base, divisor & 0xFF);

            cmd = x86_64::io::inb(this->base + x86_64::serial::line_control);
            bitops<uint8_t>::bit_clear(cmd, x86_64::serial::line_control_dlab);
            x86_64::io::outb(this->base + x86_64::serial::line_control, cmd);
        }
        uint16_t base;
        x86_64::spinlock::mutex mutex;
    };
} // x86_64::serial

#endif 