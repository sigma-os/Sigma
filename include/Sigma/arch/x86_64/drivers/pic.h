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


    void set_base_vector(uint8_t base);

    void remap(uint8_t pic1_base, uint8_t pic2_base);

    void disable();

    void enable();

    void send_eoi();

} // x86_64::pic


#endif