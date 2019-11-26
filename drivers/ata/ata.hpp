#pragma once

#include <stdint.h>
#include <utility>


namespace ata {
    constexpr std::pair<uint16_t, uint16_t> isa_ata1_base = {0x1F0, 0x3F4};
    constexpr std::pair<uint16_t, uint16_t> isa_ata2_base = {0x170, 0x374};

    namespace command_registers {
        constexpr uint16_t data = 0;

        constexpr uint16_t error = 1;
        constexpr uint16_t features = 1;
        constexpr uint16_t sector_count = 2;

        constexpr uint16_t sector_number = 3;
        constexpr uint16_t lba_low = 3;

        constexpr uint16_t cylinder_low = 4;
        constexpr uint16_t lba_mid = 4;

        constexpr uint16_t cylinder_high = 5;
        constexpr uint16_t lba_high = 5;

        constexpr uint16_t drive_select = 6;

        constexpr uint16_t status = 7;
        constexpr uint16_t command = 7;
    }

    namespace control_registers {
        constexpr uint16_t alt_status = 2;
        constexpr uint16_t device_control = 2;
        constexpr uint16_t drive_address = 3;
    }

    namespace regs {
        union status {
            struct {
                uint8_t error : 1;
                uint8_t obsolete : 2;
                uint8_t drq : 1;
                uint8_t command_specific : 1;
                uint8_t drive_fault : 1;
                uint8_t ready : 1;
                uint8_t busy : 1;
            };
            uint8_t raw;

            struct raw_bits {
                static constexpr uint8_t error = 0;
                static constexpr uint8_t drq = 3;
                static constexpr uint8_t ready = 6;
                static constexpr uint8_t busy = 7;
            };
        };
        static_assert(sizeof(status) == 1);

        union drive_select {
            struct {
                uint8_t lba28_high_nibble : 4;
                uint8_t channel_select : 1;
                uint8_t obsolete : 1;
                uint8_t chs_lba : 1;
                uint8_t obsolete1 : 1;
            };
            uint8_t raw;
        };
        static_assert(sizeof(drive_select) == 1);   

        union device_control {
            struct {
                uint8_t reserved : 1;
                uint8_t irq_enable : 1;
                uint8_t reset : 1;
                uint8_t reserved1 : 4;
                uint8_t high_order_byte : 1;
            };
            uint8_t raw;
        };
        static_assert(sizeof(device_control) == 1);
    };

    namespace commands {
        constexpr uint8_t read = 0x20;
        constexpr uint8_t write = 0x30;

        constexpr uint8_t read_dma = 0xC8;
        constexpr uint8_t write_dma = 0xCA;

        constexpr uint8_t identify = 0xEC;
    }

    class controller {
        public:
        controller(std::pair<uint16_t, uint16_t> base): base(base) {
            this->init();
        }

        private:
        struct disk {
            bool slave;
            struct {
                uint64_t exists : 1;
                uint64_t lba : 1;
                uint64_t lba48 : 1;
                uint64_t pio32 : 1;
            } flags;

            std::pair<uint8_t, uint8_t> id;

            uint8_t identity[512];
        };


        void init();
        void init_drive(disk& device);

        uint8_t read_cmd(uint8_t reg);
        uint8_t read_control(uint8_t reg);

        void write_cmd(uint8_t reg, uint8_t val);
        void write_control(uint8_t reg, uint8_t val);

        void ns_wait();
        bool wait_busy_clear();
        bool wait_for_clear(uint8_t bit);
        bool wait_for_set(uint8_t bit);

        void identify_16(uint8_t* data, disk& device);
        void identify_32(uint8_t* data, disk& device);

        bool check_identity(uint8_t* data);

        void select_drive(bool slave, bool lba, uint8_t lba28_nibble);

        std::pair<uint16_t, uint16_t> base;

        
        disk ports[2];
    };
}