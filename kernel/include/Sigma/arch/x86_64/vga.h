#ifndef SIGMA_ARCH_X86_64_VGA
#define SIGMA_ARCH_X86_64_VGA

#include <Sigma/common.h>

#include <Sigma/arch/x86_64/io.h>
#include <Sigma/arch/x86_64/misc/spinlock.h>

#include <klibc/string.h>

namespace x86_64::vga
{
    enum class text_colour {black = 0, blue = 1, green = 2, cyan = 3, red = 4, magenta = 5, brown = 7, light_gray = 7, dark_gray = 8, light_blue = 9, light_green = 10, light_cyan = 11, light_red = 12, pink = 13, yellow = 14, white = 15};

    struct PACKED_ATTRIBUTE text_entry_t
    {
        text_entry_t(uint8_t ascii, text_colour foreground, text_colour background): ascii(ascii), \
                colour((static_cast<uint8_t>(background) << 4) | static_cast<uint8_t>(foreground)) { }
        uint8_t ascii;
        uint8_t colour;
    };

    extern const uint64_t mmio;

    constexpr uint8_t terminal_width = 80;
    constexpr uint8_t terminal_height = 25;

    constexpr uint16_t vga_hardware_cursor_command_port = 0x3D4;
    constexpr uint16_t vga_hardware_cursor_data_port = 0x3D5;

    void write_entry(text_entry_t character, uint8_t x, uint8_t y);

    class writer {
        public:
        writer(): x(0), y(0), foreground(vga::text_colour::white), background(vga::text_colour::black), \
                  mutex(x86_64::spinlock::mutex()) { }

        void print(const char* str);
        void nprint(const char* str, size_t n);
        void print_char(const char c);

        void set_foreground(vga::text_colour colour);
        void set_background(vga::text_colour colour);

        void set_cursor(uint8_t x, uint8_t y);

        

        int8_t x, y;
        vga::text_colour foreground, background;

        private:

        x86_64::spinlock::mutex mutex;

        void scroll();

        void enable_hardware_cursor();
        void disable_hardware_cursor();
        void update_hardware_cursor();
    };

    
} // vga


#endif
