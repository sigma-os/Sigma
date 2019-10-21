#ifndef SIGMA_KERNEL_ARCH_X86_64_PAGING
#define SIGMA_KERNEL_ARCH_X86_64_PAGING

#include <Sigma/common.h>
#include <klibc/stdio.h>
#include <Sigma/mm/pmm.h>

constexpr uint64_t map_page_flags_present = (1 << 0);
constexpr uint64_t map_page_flags_writable = (1 << 1);
constexpr uint64_t map_page_flags_user = (1 << 2);
constexpr uint64_t map_page_flags_no_execute = (1 << 3);
constexpr uint64_t map_page_flags_cache_disable = (1 << 4);
constexpr uint64_t map_page_flags_global = (1 << 5);

namespace x86_64::paging
{
    constexpr uint64_t paging_structures_n_entries = 512;


    struct PACKED_ATTRIBUTE pml4 {
        uint64_t entries[paging_structures_n_entries];
    };
    static_assert(sizeof(pml4) == mm::pmm::block_size);

    struct PACKED_ATTRIBUTE pdpt {
        uint64_t entries[paging_structures_n_entries];
    };
    static_assert(sizeof(pdpt) == mm::pmm::block_size);

    struct PACKED_ATTRIBUTE pd {
        uint64_t entries[paging_structures_n_entries];
    };
    static_assert(sizeof(pd) == mm::pmm::block_size);

    struct PACKED_ATTRIBUTE pt {
        uint64_t entries[paging_structures_n_entries];
    };
    static_assert(sizeof(pt) == mm::pmm::block_size);

    constexpr uint64_t page_entry_present = 0;
    constexpr uint64_t page_entry_writeable = 1;
    constexpr uint64_t page_entry_user = 2;
    constexpr uint64_t page_entry_write_through = 3;
    constexpr uint64_t page_entry_cache_disable = 4;
    constexpr uint64_t page_entry_accessed = 5;
    constexpr uint64_t page_entry_dirty = 6;
    constexpr uint64_t page_entry_huge = 7;
    constexpr uint64_t page_entry_global = 8;
    constexpr uint64_t page_entry_no_execute = 63;

    class paging  {
        public:
            paging(): paging_info(nullptr) {}
            ~paging() {}
            void init();
            void deinit();

            bool map_page(uint64_t phys, uint64_t virt, uint64_t flags);

            uint64_t get_phys(uint64_t virt);

            void set();

            void clone_paging_info(x86_64::paging::paging& new_info);

            uint64_t get_paging_info();

            uint64_t get_free_range(uint64_t base, uint64_t end, size_t size);
        private:
            // Virtual address!
            pml4* paging_info; 
    };

    x86_64::paging::pml4* get_current_info();
    void set_current_info(x86_64::paging::pml4* info);
    void invalidate_addr(uint64_t addr);


} // x86_64::paging


#endif