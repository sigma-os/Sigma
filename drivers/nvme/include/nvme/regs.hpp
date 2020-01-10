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
    constexpr uint8_t identify_opcode = 0x6;

    struct PACKED identify_info {
        uint16_t pci_vendor_id;
        uint16_t pci_subsystem_vendor_id;
        char serial_number[20];
        char model_number[40];
        char fw_revision[8];
        uint8_t rab;
        char ieee_oui_id[3];
        uint8_t cmic;
        uint8_t mtds;
        uint16_t controller_id;
        uint32_t version;
        uint32_t rtd3r;
        uint32_t rtd3e;
        uint32_t oaes;
        uint32_t ctratt;
        uint16_t rrls;
        uint8_t reserved[9];
        uint8_t cntrltype;
        uint8_t fguid[16];
        uint16_t crdt1;
        uint16_t crdt2;
        uint16_t crdt3;
        uint8_t reserved_1[106];
        uint8_t management_interface[16];
        uint16_t oacs;
        uint8_t acl;
        uint8_t aerl;
        uint8_t frmw;
        uint8_t lpa;
        uint8_t elpe;
        uint8_t npss;
        uint8_t avscc;
        uint8_t apsta;
        uint16_t wctemp;
        uint16_t cctemp;
        uint16_t mtfa;
        uint32_t hmpre;
        uint32_t hmmin;
        uint8_t tnvmcap[16];
        uint8_t unvmcap[16];
        uint32_t rpmbs;
        uint16_t edstt;
        uint8_t dsto;
        uint8_t fwug;
        uint16_t kas;
        uint16_t hctma;
        uint16_t mntmt;
        uint16_t mxtmt;
        uint32_t sanicap;
        uint32_t hmminds;
        uint16_t hmmaxd;
        uint16_t nsetidmax;
        uint16_t endgidmax;
        uint8_t anatt;
        uint8_t anacap;
        uint32_t anagrpmax;
        uint32_t nanagrpid;
        uint32_t pels;
        uint8_t reserved_2[156];
        uint8_t sqes;
        uint8_t cqes;
        uint16_t maxcmd;
        uint32_t nn;
        uint16_t oncs;
        uint16_t fuses;
        uint8_t fna;
        uint8_t vwc;
        uint16_t awun;
        uint16_t awupf;
        uint8_t nvscc;
        uint8_t nwpc;
        uint16_t acwu;
        uint16_t reserved_3;
        uint32_t sgls;
        uint32_t mnan;
        uint8_t reserved_4[224];
        char subnqn[256];
        uint8_t reserved_5[768];
        uint8_t over_fabrics[256];
        uint8_t psd0[32][32]; // 32 PSDs in total
        uint8_t vendor_specific[1024];
    };
    static_assert(sizeof(identify_info) == 0x1000);

    struct PACKED create_completion_queue_command {
        command_header header;
        struct {
            uint32_t qid : 16;
            uint32_t size : 16;
        };
        struct {
            uint32_t pc : 1;
            uint32_t irq_enable : 1;
            uint32_t reserved : 14;
            uint32_t irq_vector : 16;
        };
        uint32_t reserved_1[4];
    };
    static_assert(sizeof(create_completion_queue_command) == sizeof(command));
    constexpr uint8_t create_completion_queue_opcode = 0x05;

    struct PACKED create_submission_queue_command {
        command_header header;
        struct {
            uint32_t qid : 16;
            uint32_t size : 16;
        };
        struct {
            uint32_t pc : 1;
            uint32_t priority : 2;
            uint32_t reserved : 13;
            uint32_t cqid : 16;
        };
        uint32_t reserved_1[4];
    };
    static_assert(sizeof(create_submission_queue_command) == sizeof(command));
    constexpr uint8_t create_submission_queue_opcode = 0x01;

    struct PACKED set_features_command {
        command_header header;
        struct {
            uint32_t fid : 8;
            uint32_t reserved : 23;
            uint32_t save : 1;
        };
        uint32_t data;
        uint32_t reserved_1[4];
    };
    static_assert(sizeof(set_features_command) == sizeof(command));
    constexpr uint8_t set_features_opcode = 0x09;

    constexpr uint8_t n_queues_fid = 0x07;

    struct PACKED read_command {
        command_header header;
        uint64_t start_lba;
        struct {
            uint32_t n_sectors : 16;
            uint32_t reserved : 10;
            uint32_t prinfo : 4;
            uint32_t fua : 1;
            uint32_t lr : 1;
        };
        struct {
            uint32_t dsm : 8;
            uint32_t reserved_1 : 24;
        };
        uint32_t eilbrt;
        struct {
            uint32_t elbatm : 16;
            uint32_t elbat : 16;
        };
    };
    static_assert(sizeof(read_command) == sizeof(command));
    constexpr uint8_t read_opcode = 0x02;
}