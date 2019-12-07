#pragma once

#include <cstdint>
#include <cstddef>

#define PACKED [[gnu::packed]]

namespace ahci
{
    namespace regs
    {
        struct PACKED ghcr_t 
        {
            union cap_t {
                cap_t(uint32_t cap): raw(cap) {}
                struct {
                    uint32_t np : 5;
                    uint32_t sxs : 1;
                    uint32_t ems : 1;
                    uint32_t cccs : 1;
                    uint32_t ncs : 5;
                    uint32_t psc : 1;
                    uint32_t ssc : 1;
                    uint32_t pmd : 1;
                    uint32_t fbss : 1;
                    uint32_t spm : 1;
                    uint32_t sam : 1;
                    uint32_t reserved : 1;
                    uint32_t iss : 4;
                    uint32_t sclo : 1;
                    uint32_t sal : 1;
                    uint32_t salp : 1;
                    uint32_t sss : 1;
                    uint32_t smps : 1;
                    uint32_t ssntf : 1;
                    uint32_t sncq : 1;
                    uint32_t s64a : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(cap_t) == 4);
            volatile uint32_t cap;

            union ghc_t {
                ghc_t(uint32_t ghc): raw(ghc) {}
                struct {
                    uint32_t hr : 1;
                    uint32_t ie : 1;
                    uint32_t mrsm : 1;
                    uint32_t reserved : 28;
                    uint32_t ea : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ghc_t) == 4);
            volatile uint32_t ghc;

            union is_t {
                is_t(uint32_t is): irq_pending(is) {}
                uint32_t irq_pending;
                bool is_pending(uint32_t port){
                    return ((irq_pending & (uint32_t{1} << port)) == 0) ? false : true;
                }
            };
            static_assert(sizeof(is_t) == 4);
            volatile uint32_t is;

            union pi_t {
                pi_t(uint32_t pi): implemented(pi) {}
                uint32_t implemented;
                bool is_implemented(uint32_t port){
                    return ((implemented & (uint32_t{1} << port)) == 0) ? false : true;
                }
            };
            static_assert(sizeof(pi_t) == 4);
            volatile uint32_t pi;

            union vs_t {
                vs_t(uint32_t vs): raw(vs) {}
                struct {
                    uint32_t subminor : 8;
                    uint32_t minor : 8;
                    uint32_t major : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(vs_t) == 4);
            volatile uint32_t vs;

            union ccc_ctl_t {
                ccc_ctl_t(uint32_t ccc_ctl): raw(ccc_ctl) {}
                struct {
                    uint32_t en : 1;
                    uint32_t reserved : 2;
                    uint32_t interrupt : 5;
                    uint32_t cc : 8;
                    uint32_t tv : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ccc_ctl_t) == 4);
            volatile uint32_t ccc_ctl;

            union ccc_ports_t {
                ccc_ports_t(uint32_t ccc_ports): raw(ccc_ports) {}
                struct {
                    uint32_t ports;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ccc_ports_t) == 4);
            volatile uint32_t ccc_ports;

            union em_loc_t {
                em_loc_t(uint32_t em_loc): raw(em_loc) {}
                struct {
                    uint32_t size : 16;
                    uint32_t offset : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(em_loc_t) == 4);
            volatile uint32_t em_loc;

            union em_ctl_t {
                em_ctl_t(uint32_t em_ctl): raw(em_ctl) {}
                struct {
                    uint32_t mr : 1;
                    uint32_t reserved : 7;
                    uint32_t tm : 1;
                    uint32_t reset : 1;
                    uint32_t reserved_0 : 6;
                    uint32_t led : 1;
                    uint32_t safte : 1;
                    uint32_t ses2 : 1;
                    uint32_t sgpio : 1;
                    uint32_t reserved_1 : 4;
                    uint32_t smb : 1;
                    uint32_t xmt : 1;
                    uint32_t alhd : 1;
                    uint32_t pm : 1;
                };
                uint32_t raw;   
            };
            static_assert(sizeof(em_ctl_t) == 4);
            volatile uint32_t em_ctl;

            union cap2_t {
                cap2_t(uint32_t cap2): raw(cap2) {}
                struct {
                    uint32_t boh : 1;
                    uint32_t nvmp : 1;
                    uint32_t apst : 1;
                    uint32_t sds : 1;
                    uint32_t sadm : 1;
                    uint32_t deso : 1;
                    uint32_t reserved : 26;
                };
                uint32_t raw;
            };
            static_assert(sizeof(cap2_t) == 4);
            volatile uint32_t cap2;

            union bohc_t {
                bohc_t(uint32_t bohc): raw(bohc) {}
                struct {
                    uint32_t bos : 1;
                    uint32_t oos : 1;
                    uint32_t sooe : 1;
                    uint32_t ooc : 1;
                    uint32_t bb : 1;
                    uint32_t reserved : 27;
                };
                uint32_t raw;
            };
            static_assert(sizeof(bohc_t) == 4);
            volatile uint32_t bohc;
        };
        static_assert(sizeof(ghcr_t) == 44);
    
        struct PACKED prs_t {
            union clb_t {
                struct {
                    uint32_t reserved : 10;
                    uint32_t base : 22;
                };
                uint32_t raw;
            };
            static_assert(sizeof(clb_t) == 4);
            volatile uint32_t clb;

            union clbu_t {
                clbu_t(uint32_t clbu): base_high(clbu) {}
                uint32_t base_high;
            };
            static_assert(sizeof(clbu_t) == 4);
            volatile uint32_t clbu;

            union fb_t {
                fb_t(uint32_t fb): raw(fb) {}
                struct {
                    uint32_t reserved : 8;
                    uint32_t base : 24;
                };
                uint32_t raw;
            };
            static_assert(sizeof(fb_t) == 4);
            volatile uint32_t fb;


            union fbu_t {
                fbu_t(uint32_t fbu): base_high(fbu) {}
                uint32_t base_high;
            };
            static_assert(sizeof(fbu_t) == 4);
            volatile uint32_t fbu;

            union is_t {
                is_t(uint32_t is): raw(is) {}
                struct {
                    uint32_t dhrs : 1;
                    uint32_t pss : 1;
                    uint32_t dss : 1;
                    uint32_t sdbs : 1;
                    uint32_t ufs : 1;
                    uint32_t pds : 1;
                    uint32_t pcs : 1; 
                    uint32_t dmps : 1;
                    uint32_t reserved : 14;
                    uint32_t prcs : 1;
                    uint32_t imps : 1;
                    uint32_t ofs : 1;
                    uint32_t reserved_0 : 1;
                    uint32_t infs : 1;
                    uint32_t ifs : 1;
                    uint32_t hbds : 1;
                    uint32_t hbfs : 1;
                    uint32_t tfes : 1;
                    uint32_t cpds : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(is_t) == 4);
            volatile uint32_t is;
            
            union ie_t {
                ie_t(uint32_t ie): raw(ie) {}
                struct {
                    uint32_t dhre : 1;
                    uint32_t pse : 1;
                    uint32_t dse : 1;
                    uint32_t sdbe : 1;
                    uint32_t ufe : 1;
                    uint32_t pde : 1;
                    uint32_t pce : 1; 
                    uint32_t dmpe : 1;
                    uint32_t reserved : 14;
                    uint32_t prce : 1;
                    uint32_t impe : 1;
                    uint32_t ofe : 1;
                    uint32_t reserved_0 : 1;
                    uint32_t infe : 1;
                    uint32_t ife : 1;
                    uint32_t hbde : 1;
                    uint32_t hbfe : 1;
                    uint32_t tfee : 1;
                    uint32_t cpde : 1;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ie_t) == 4);
            volatile uint32_t ie;

            union cmd_t {
                cmd_t(uint32_t cmd): raw(cmd) {}
                struct {
                    uint32_t st : 1;
                    uint32_t sud : 1;
                    uint32_t pod : 1;
                    uint32_t clo : 1;
                    uint32_t fre : 1;
                    uint32_t reserved : 3;
                    uint32_t ccs : 5;
                    uint32_t mpss : 1;
                    uint32_t fr : 1;
                    uint32_t cr : 1;
                    uint32_t cps : 1;
                    uint32_t pma : 1;
                    uint32_t hpcp : 1;
                    uint32_t mpsp : 1;
                    uint32_t cpd : 1;
                    uint32_t esp : 1;
                    uint32_t fbscp : 1;
                    uint32_t apste : 1;
                    uint32_t atapi : 1;
                    uint32_t dlae : 1;
                    uint32_t alpe : 1;
                    uint32_t asp : 1;
                    uint32_t icc : 4;
                };
                uint32_t raw;
            };
            static_assert(sizeof(cmd_t) == 4);
            volatile uint32_t cmd;

            volatile uint32_t reserved;

            union tfd_t {
                tfd_t(uint32_t tfd): raw(tfd) {}
                struct {
                    uint32_t sts : 8;
                    uint32_t err : 8;
                    uint32_t  reserved : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(tfd_t) == 4);
            volatile uint32_t tfd;

            union sig_t {
                sig_t(uint32_t sig): raw(sig) {}
                struct {
                    uint32_t sector_count : 8;
                    uint32_t lba_low : 8;
                    uint32_t lba_mid : 8;
                    uint32_t lba_high : 8;
                };
                uint32_t raw;

                enum class device_types : uint32_t {
                    Nothing = 0x0,
                    SATA = 0x0101,
                    SATAPI = 0xEB140101,
                    EnclosureManagementBridge = 0xC33C0101,
                    PortMultiplier = 0x96690101,
                    Default = 0xFFFFFFFF
                };
                device_types get_type(){
                    switch (raw)
                    {
                    case static_cast<uint32_t>(device_types::Nothing):
                        return device_types::Nothing;

                    case static_cast<uint32_t>(device_types::SATA):
                        return device_types::SATA;

                    case static_cast<uint32_t>(device_types::SATAPI):
                        return device_types::SATAPI;

                    case static_cast<uint32_t>(device_types::EnclosureManagementBridge):
                        return device_types::EnclosureManagementBridge;

                    case static_cast<uint32_t>(device_types::PortMultiplier):
                        return device_types::PortMultiplier;

                    case static_cast<uint32_t>(device_types::Default):
                        return device_types::Default;
                    
                    default:
                        return device_types::Nothing;
                    }
                }
            };
            static_assert(sizeof(sig_t) == 4);
            volatile uint32_t sig;

            union ssts_t {
                ssts_t(uint32_t ssts): raw(ssts) {}
                struct {
                    uint32_t det : 4;
                    uint32_t spd : 4;
                    uint32_t ipm : 4;
                    uint32_t reserved : 20;
                };
                uint32_t raw;
            };
            static_assert(sizeof(ssts_t) == 4);
            volatile uint32_t ssts;

            union sctl_t {
                sctl_t(uint32_t sctl): raw(sctl) {}
                struct {
                    uint32_t det : 4;
                    uint32_t spd : 4;
                    uint32_t ipm : 4;
                    uint32_t spm : 4;
                    uint32_t pmp : 4;
                    uint32_t reserved : 12;
                };
                uint32_t raw;
            };
            static_assert(sizeof(sctl_t) == 4);
            volatile uint32_t sctl;

            union serr_t {
                serr_t(uint32_t serr): raw(serr) {}
                struct {
                    uint32_t err : 16;
                    uint32_t diag : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(serr_t) == 4);
            volatile uint32_t serr;

            union sact_t {
                sact_t(uint32_t sact): status(sact) {}
                uint32_t status;
            };
            static_assert(sizeof(sact_t) == 4);
            volatile uint32_t sact;

            union ci_t {
                ci_t(uint32_t ci): command_issued(ci) {}
                uint32_t command_issued;
            };
            static_assert(sizeof(ci_t) == 4);
            volatile uint32_t ci;

            union sntf_t {
                sntf_t(uint32_t sntf): raw(sntf) {};
                struct {
                    uint32_t pmn : 16;
                    uint32_t reserved : 16;
                };
                uint32_t raw;
            };
            static_assert(sizeof(sntf_t) == 4);
            volatile uint32_t sntf;

            union fbs_t {
                fbs_t(uint32_t fbs): raw(fbs) {}
                struct {
                    uint32_t en : 1;
                    uint32_t dec : 1;
                    uint32_t sde : 1;
                    uint32_t reserved : 5;
                    uint32_t dev : 4;
                    uint32_t ado : 4;
                    uint32_t dwe : 4;
                    uint32_t reserved_0 : 12;    
                };
                uint32_t raw;
                
            };
            static_assert(sizeof(fbs_t) == 4);
            volatile uint32_t fbs;

            union dev_sleep_t {
                dev_sleep_t(uint32_t dev_sleep): raw(dev_sleep) {}
                struct {
                    uint32_t adse : 1;
                    uint32_t dsp : 1;
                    uint32_t deto : 8;
                    uint32_t mdat : 5;
                    uint32_t dito : 10;
                    uint32_t reserved : 3;
                };
                uint32_t raw;
            };
            static_assert(sizeof(dev_sleep_t) == 4);
            volatile uint32_t dev_sleep;

            std::byte reserved_0[40];
            std::byte vendor_specific[16];
        };
        static_assert(sizeof(prs_t) == 128);

        struct PACKED hba_t
        {
            ghcr_t ghcr;
            std::byte reserved[52];
            std::byte nvmhci[64];
            std::byte vendor_specific[96];
            prs_t ports[];
        };

        struct PACKED cmd_header {
            struct {
                uint32_t cfl : 5;
                uint32_t atapi : 1;
                uint32_t write : 1;
                uint32_t prefetchable : 1;
                uint32_t srst_control : 1;
                uint32_t bist : 1;
                uint32_t clear : 1;
                uint32_t reserved : 1;
                uint32_t pmp : 4;
                uint32_t prdtl : 16;
            } flags;

            uint32_t prdbc;
            uint32_t ctba;
            uint32_t ctbau;
            std::byte reserved[16];
            static_assert(sizeof(flags) == 4);
        };
        static_assert(sizeof(cmd_header) == 32);

        struct PACKED h2d_register_fis {
            uint8_t type = 0x27;
            uint8_t flags;
            uint8_t command;
            uint8_t features;
            uint8_t lba_0;
            uint8_t lba_1;
            uint8_t lba_2;
            uint8_t dev_head;
            uint8_t lba_3;
            uint8_t lba_4;
            uint8_t lba_5;
            uint8_t features_exp;
            uint8_t sector_count_low;
            uint8_t sector_count_high;
            uint8_t reserved;
            uint8_t control;
            std::byte reserved_0[4];
        };
        static_assert(sizeof(h2d_register_fis) == 20);

        struct PACKED prdt {
            uint32_t low;
            uint32_t high;
            uint32_t reserved;
            struct {
                uint32_t byte_count : 22;
                uint32_t reserved : 9;
                uint32_t irq_on_completion : 1;
            } flags;
            static_assert(sizeof(flags) == 4);
        };
        static_assert(sizeof(prdt) == 16);
    } // namespace regs

    class controller {
        public:
        controller(uintptr_t phys_base, size_t size);


        private:
        bool bios_gain_ownership();

        volatile regs::hba_t* base;
        uint8_t major_version, minor_version, subminor_version, n_allocated_ports, n_command_slots;
        bool addressing_64bit;

        struct port {
            volatile regs::prs_t* regs;
            regs::prs_t::sig_t::device_types type;

            union phys_region {
                struct {
                    regs::cmd_header command_headers[32]; // Allocate 32 even if there are less supported, we have to allocate 4KiB anyway
                    std::byte receive_fis[256]; // TODO: What is this?
                };
                std::byte pad[0x1000];
            };


            phys_region* region;
            void wait_idle();
        };

        port* ports;
    };
} // namespace ahci
