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
            PACKED struct {
                uint32_t np : 5;
                uint32_t sxs : 1;
                uint32_t ems : 1;
                uint32_t cccs : 1;
                uint32_t ncs : 1;
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
            } cap;
            static_assert(sizeof(cap) == 4);

            PACKED struct
            {
                uint32_t hr : 1;
                uint32_t ie : 1;
                uint32_t mrsm : 1;
                uint32_t reserved : 28;
                uint32_t ea : 1;
            } ghc;
            static_assert(sizeof(ghc) == 4);

            PACKED struct {
                uint32_t irq_pending;
                bool is_pending(uint32_t port){
                    return ((irq_pending & (uint32_t{1} << port)) == 0) ? false : true;
                }
            } is;
            static_assert(sizeof(is) == 4);

            PACKED struct {
                uint32_t implemented;
                bool is_implemented(uint32_t port){
                    return ((implemented & (uint32_t{1} << port)) == 0) ? false : true;
                }
            } pi;
            static_assert(sizeof(pi) == 4);

            PACKED struct {
                uint32_t minor : 8;
                uint32_t patch : 8;
                uint32_t major : 16;
            } vs;
            static_assert(sizeof(vs) == 4);

            PACKED struct {
                uint32_t en : 1;
                uint32_t reserved : 2;
                uint32_t interrupt : 5;
                uint32_t cc : 8;
                uint32_t tv : 16;
            } ccc_ctl;
            static_assert(sizeof(ccc_ctl) == 4);

            PACKED struct {
                uint32_t ports;
            } ccc_ports;
            static_assert(sizeof(ccc_ports) == 4);

            PACKED struct {
                uint32_t size : 16;
                uint32_t offset : 16;
            } em_loc;
            static_assert(sizeof(em_loc) == 4);

            PACKED struct {
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
            } em_ctl;
            static_assert(sizeof(em_ctl) == 4);
            
            PACKED struct {
                uint32_t boh : 1;
                uint32_t nvmp : 1;
                uint32_t apst : 1;
                uint32_t sds : 1;
                uint32_t sadm : 1;
                uint32_t deso : 1;
                uint32_t reserved : 26;
            } cap2;
            static_assert(sizeof(cap2) == 4);
            
            PACKED struct {
                uint32_t bos : 1;
                uint32_t oos : 1;
                uint32_t sooe : 1;
                uint32_t ooc : 1;
                uint32_t bb : 1;
                uint32_t reserved : 27;
            } bohc;
            static_assert(sizeof(bohc) == 4);
        };
        static_assert(sizeof(ghcr_t) == 44);
    
        struct PACKED prs_t {
            PACKED struct {
                uint32_t reserved : 10;
                uint32_t base : 22;
            } clb;
            static_assert(sizeof(clb) == 4);

            PACKED struct {
                uint32_t base : 32;
            } clbu;
            static_assert(sizeof(clbu) == 4);

            PACKED struct {
                uint32_t reserved : 8;
                uint32_t base : 24;
            } fb;
            static_assert(sizeof(fb) == 4);

            PACKED struct {
                uint32_t base : 32;
            } fbu;
            static_assert(sizeof(fbu) == 4);

            PACKED struct {
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
            } is;
            static_assert(sizeof(is) == 4);
            
            PACKED struct {
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
            } ie;
            static_assert(sizeof(ie) == 4);

            PACKED struct {
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
            } cmd;
            static_assert(sizeof(cmd) == 4);

            uint32_t reserved;

            PACKED struct {
                uint32_t sts : 8;
                uint32_t err : 8;
                uint32_t  reserved : 16;
            } tfd;
            static_assert(sizeof(tfd) == 4);

            PACKED union {
                struct PACKED {
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
            } sig;
            static_assert(sizeof(sig) == 4);

            PACKED struct {
                uint32_t det : 4;
                uint32_t spd : 4;
                uint32_t ipm : 4;
                uint32_t reserved : 20;
            } ssts;
            static_assert(sizeof(ssts) == 4);

            PACKED struct {
                uint32_t det : 4;
                uint32_t spd : 4;
                uint32_t ipm : 4;
                uint32_t spm : 4;
                uint32_t pmp : 4;
                uint32_t reserved : 12;
            } sctl;
            static_assert(sizeof(sctl) == 4);

            PACKED struct {
                uint32_t err : 16;
                uint32_t diag : 16;
            } serr;
            static_assert(sizeof(serr) == 4);

            PACKED struct {
                uint32_t status;
            } sact;
            static_assert(sizeof(sact) == 4);

            PACKED struct {
                uint32_t command_issued;
            } ci;
            static_assert(sizeof(ci) == 4);

            PACKED struct {
                uint32_t pmn : 16;
                uint32_t reserved : 16;
            } sntf;
            static_assert(sizeof(sntf) == 4);

            PACKED struct {
                uint32_t en : 1;
                uint32_t dec : 1;
                uint32_t sde : 1;
                uint32_t reserved : 5;
                uint32_t dev : 4;
                uint32_t ado : 4;
                uint32_t dwe : 4;
                uint32_t reserved_0 : 12;
            } fbs;
            static_assert(sizeof(fbs) == 4);

            PACKED struct {
                uint32_t adse : 1;
                uint32_t dsp : 1;
                uint32_t deto : 8;
                uint32_t mdat : 5;
                uint32_t dito : 10;
                uint32_t reserved : 3;
            } dev_sleep;
            static_assert(sizeof(dev_sleep) == 4);

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
    } // namespace regs

    class controller {
        public:
        controller(uintptr_t phys_base, size_t size);


        private:
        regs::hba_t* base;
    };
} // namespace ahci
