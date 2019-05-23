#ifndef SIGMA_KERNEL_X86_64_PIT
#define SIGMA_KERNEL_X86_64_PIT

#include <Sigma/common.h>
#include <Sigma/arch/x86_64/io.h>

namespace x86_64::pit
{
    constexpr uint16_t channel_0_data_port = 0x40;
    constexpr uint16_t channel_1_data_port = 0x41;
    constexpr uint16_t channel_2_data_port = 0x42;
    constexpr uint16_t command_port = 0x43;

    constexpr uint64_t tick_speed = 1193180;

    void setup_sleep(uint64_t hz);
    void msleep_poll();
} // namespace x86_64::pit


#endif