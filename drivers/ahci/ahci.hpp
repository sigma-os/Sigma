#pragma once

#include <cstdint>
#include <cstddef>

namespace ahci
{
    namespace regs
    {
        struct ghcr_t
        {
            struct {
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

            struct
            {
                uint32_t hr : 1;
                uint32_t ie : 1;
                uint32_t mrsm : 1;
                uint32_t reserved : 28;
                uint32_t ea : 1;
            } ghc;
            static_assert(sizeof(ghc) == 4);

            struct {
                uint32_t irq_pending;
                bool is_pending(uint32_t port){
                    return ((irq_pending & (uint32_t{1} << port)) == 0) ? false : true;
                }
            } is;
            static_assert(sizeof(is) == 4);

            struct {
                uint32_t implemented;
                bool is_implemented(uint32_t port){
                    return ((implemented & (uint32_t{1} << port)) == 0) ? false : true;
                }
            } pi;
            static_assert(sizeof(pi) == 4);

            struct {
                uint32_t major : 16;
                uint32_t minor : 8;
                uint32_t patch : 8;
            } vs;
            static_assert(sizeof(vs) == 4);

            struct {
                uint32_t en : 1;
                uint32_t reserved : 2;
                uint32_t interrupt : 5;
                uint32_t cc : 8;
                uint32_t tv : 16;
            } ccc_ctl;
            static_assert(sizeof(ccc_ctl) == 4);

            struct {
                uint32_t ports;
            } ccc_ports;
            static_assert(sizeof(ccc_ports) == 4);

            struct {
                uint32_t size : 16;
                uint32_t offset : 16;
            } em_loc;
            static_assert(sizeof(em_loc) == 4);

            struct {
                uint32_t mr : 1;
                uint32_t reserved : 7;
                uint32_t tm : 1;
                uint32_t reset : 1;
                uint32_t reserved : 6;
                uint32_t led : 1;
                uint32_t safte : 1;
                uint32_t ses2 : 1;
                uint32_t sgpio : 1;
                uint32_t reserved_0 : 4;
                uint32_t smb : 1;
                uint32_t xmt : 1;
                uint32_t alhd : 1;
                uint32_t pm : 1;
            } em_ctl;
            static_assert(sizeof(em_ctl) == 4);
            
            struct {
                uint32_t boh : 1;
                uint32_t nvmp : 1;
                uint32_t apst : 1;
                uint32_t sds : 1;
                uint32_t sadm : 1;
                uint32_t deso : 1;
                uint32_t reserved : 26;
            } cap2;
            static_assert(sizeof(cap2) == 4);
            
            struct {
                uint32_t bos : 1;
                uint32_t oos : 1;
                uint32_t sooe : 1;
                uint32_t ooc : 1;
                uint32_t bb : 1;
                uint32_t reserved : 27;
            } bohc;
            static_assert(sizeof(bohc) == 4);
        };
        static_assert(sizeof(ghcr) == 44);

        struct hba_t
        {
            ghcr_t ghcr;
            std::byte reserved[52];
            std::byte nvmhci[64];
            std::byte vendor_specific[96];
            // TODO: Port register sets
        };
    } // namespace regs
} // namespace ahci
