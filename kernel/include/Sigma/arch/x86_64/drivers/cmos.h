#ifndef SIGMA_KERNEL_X86_64_CMOS
#define SIGMA_KERNEL_X86_64_CMOS

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/io.h>

namespace x86_64::cmos
{
    constexpr uint16_t select_reg = 0x70;
    constexpr uint16_t data_reg = 0x71;

    constexpr uint8_t reg_reset_code = 0xF;

    constexpr uint8_t reg_reset_code_jump = 0xA;



    uint8_t read(uint8_t reg);
    void write(uint8_t reg, uint8_t val);
} // x86_64::cmos


#endif