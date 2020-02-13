#ifndef SIGMA_ARCH_X86_64_SL_PAGING_H
#define SIGMA_ARCH_X86_64_SL_PAGING_H

#include <Sigma/common.h>

namespace x86_64::sl_paging
{
    union PACKED_ATTRIBUTE sl_pmle {
        struct {
            uint64_t r : 1;
            uint64_t w : 1;
            uint64_t x : 1;

            uint64_t reserved : 5;
            uint64_t accessed : 1;
            uint64_t reserved_0 : 3;
            uint64_t addr : 40;
            uint64_t reserved_1 : 12;
        };
        uint64_t raw;
    };
    static_assert(sizeof(sl_pmle) == 8);

    struct PACKED_ATTRIBUTE sl_pml {
        sl_pmle entries[512];

        sl_pmle& operator[](int i){
            return entries[i];
        }
    };
    static_assert(sizeof(sl_pml) == 0x1000);

    union PACKED_ATTRIBUTE sl_pml1e {
        struct {
            uint64_t r : 1;
            uint64_t w : 1;
            uint64_t x : 1;

            uint64_t extended_mem_type : 3;
            uint64_t ignore_pat : 1;
            uint64_t reserved : 1;
            uint64_t accessed : 1;
            uint64_t dirty : 1;
            uint64_t reserved_0 : 1;
            uint64_t snoop : 1;
            uint64_t frame : 40;
            uint64_t reserved_1 : 10;
            uint64_t transient_mapping : 1;
            uint64_t reserved_2 : 1;
        };
        uint64_t raw;
    };

    static_assert(sizeof(sl_pml1e) == 8);

    struct PACKED_ATTRIBUTE sl_pml1 {
        sl_pml1e entries[512];

        sl_pml1e& operator[](int i){
            return entries[i];
        }
    };
    static_assert(sizeof(sl_pml1) == 0x1000);

    enum {
        mapSlPageRead = (1 << 0),
        mapSlPageWrite = (1 << 1),
        mapSlPageExecute = (1 << 2),
    };

    class context {
        public:
        context(uint8_t level);
        ~context();

        void map(uint64_t pa, uint64_t iova, uint64_t flags);

        uint64_t get_ptr();

        private:
        uint8_t level;
        uint64_t phys_root;
    };
} // namespace x86_64::sl_paging


#endif