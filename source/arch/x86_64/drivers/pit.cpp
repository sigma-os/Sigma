#include <Sigma/arch/x86_64/drivers/pit.h>

constexpr uint16_t make_divider(uint64_t hz){
    return (x86_64::pit::tick_speed / hz);
}

void x86_64::pit::setup_sleep(uint64_t hz){
    x86_64::io::outb(0x61, ((x86_64::io::inb(0x61) & 0xFD) | 1)); // Bad command, explain


    x86_64::io::outb(x86_64::pit::command_port, 0b10110010); // Channel 2, lobyte/hibyte, hardware retriggreable oneshot, 16bit binary

    uint64_t divider = make_divider(hz);

    x86_64::io::outb(x86_64::pit::channel_2_data_port, (divider & 0xFF));
    x86_64::io::outb(x86_64::pit::channel_2_data_port, ((divider >> 8) & 0xFF));
}

void x86_64::pit::msleep_poll(){
    uint8_t res = x86_64::io::inb(0x61); // Bad command, explain
    x86_64::io::outb(0x61, (res & 0xFE)); // Bad command, explain
    x86_64::io::outb(0x61, ((res & 0xFE) | 1)); // Bad command, explain

    while(x86_64::io::inb(0x61) & 0x20); // Bad poll explain
}