#pragma once

#include <stdint.h>
#include <stddef.h>

#define PACKED [[gnu::packed]]

namespace nvme::regs {
    struct PACKED bar {
        union cap_t {
            cap_t(uint64_t cap): raw(cap) {}

            struct {
                uint64_t mqes : 16;
                uint64_t cqr : 1;
                uint64_t ams : 2;
                uint64_t reserved : 5;
                uint64_t to : 8;
                uint64_t dstrd : 4;
                uint64_t nssrs : 1;
                uint64_t css : 8;
                uint64_t bps : 1;
                uint64_t reserved_1 : 2;
                uint64_t mpsmin : 4;
                uint64_t mpsmax : 4;
                uint64_t pmrs : 1;
                uint64_t cmbs : 1;
                uint64_t reserved_2 : 6;
            };
            uint64_t raw;
        };
        static_assert(sizeof(cap_t) == 8);
        uint64_t cap;

        union vs_t {
            vs_t(uint32_t vs): raw(vs) {}

            struct {
                uint32_t tertiary : 8;
                uint32_t minor : 8;
                uint32_t major : 16;
            };
            uint32_t raw;
        };
        static_assert(sizeof(vs_t) == 4);
        uint32_t vs;

        uint32_t irq_mask_set;
        uint32_t irq_mask_clear;
        
        union cc_t {
            cc_t(uint32_t cc): raw(cc) {}

            struct {
                uint32_t en : 1;
                uint32_t reserved : 3;
                uint32_t css : 3;
                uint32_t mps : 4;
                uint32_t ams : 3;
                uint32_t shn : 2;
                uint32_t iosqes : 4;
                uint32_t iocqes : 4;
                uint32_t reserved_1 : 8;
            };
            uint32_t raw;
        };
        static_assert(sizeof(vs_t) == 4);
        uint32_t cc;

        uint32_t reserved;
        uint32_t controller_status;
        uint32_t subsystem_reset;

        union aqa_t {
            aqa_t(uint32_t aqa): raw(aqa) {}

            struct {
                uint32_t asqs : 12;
                uint32_t reserved : 4;
                uint32_t acqs : 12;
                uint32_t reserved_1 : 4;
            };
            uint32_t raw;
        };
        static_assert(sizeof(aqa_t) == 4);
        uint32_t aqa;
        uint64_t asq;
        uint64_t acq;
    };

    struct PACKED command_header {
        struct {
            uint32_t opcode : 8;
            uint32_t fused : 2;
            uint32_t reserved : 4;
            uint32_t psdt : 2;
            uint32_t cid : 16;
        };
        uint32_t namespace_id;
        uint64_t reserved_0;
        uint64_t metadata_pointer;
        union {
            // For PRP
            struct {
                uint64_t prp1;
                uint64_t prp2;
            };
            // For SGL
            struct {
                uint8_t sgl_segment[16];
            };
            struct {
                uint8_t padding[16];
            };
        };
    };
    static_assert(sizeof(command_header) == 40);

    struct PACKED command {
        command_header header;
        uint32_t cdw[6];
    };
    static_assert(sizeof(command) == 64);

    struct PACKED identify_command {
        command_header header;
        uint8_t cns;
        uint8_t reserved;
        uint16_t controller_id;
        uint16_t nvm_set_id;
        uint16_t reserved_1;
        uint32_t reserved_2;
        uint32_t reserved_3;
        struct {
            uint32_t uuid_index : 7;
            uint32_t reserved_4 : 25;
        };
        uint32_t reserved_5;
    };
    static_assert(sizeof(identify_command) == sizeof(command));

    struct PACKED completion {
        uint32_t command_specific;
        uint32_t reserved;
        uint16_t sq_head_pointer;
        uint16_t sq_id;
        uint16_t command_id;
        struct {
            uint16_t phase : 1;
            uint16_t code : 8;
            uint16_t code_type : 3;
            uint16_t retry_delay : 2;
            uint16_t more_info : 1;
            uint16_t do_not_retry : 1;
        } status;
    };
    static_assert(sizeof(completion) == 16);
}