#ifndef SIGMA_KERNEL_ARCH_X86_64_PAGING
#define SIGMA_KERNEL_ARCH_X86_64_PAGING

#include <Sigma/common.h>
#include <Sigma/interfaces/paging_manager.h>
#include <klibc/stdio.h>
#include <Sigma/mm/pmm.h>

namespace x86_64::paging
{
    constexpr uint64_t paging_structures_n_entries = 512;


    struct PACKED_ATTRIBUTE pml4 {
        uint64_t entries[paging_structures_n_entries];
    };

    struct PACKED_ATTRIBUTE pdpt {
        uint64_t entries[paging_structures_n_entries];
    };

    struct PACKED_ATTRIBUTE pd {
        uint64_t entries[paging_structures_n_entries];
    };

    struct PACKED_ATTRIBUTE pt {
        uint64_t entries[paging_structures_n_entries];
    };

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

    class paging : public virtual IPaging {
        public:
            paging(): paging_info(nullptr) {}
            ~paging() {this->deinit();}
            void init();
            void deinit();

            bool map_page(uint64_t phys, uint64_t virt, uint64_t flags);

            uint64_t get_phys(uint64_t virt);

            void set_paging_info();

            void clone_paging_info(IPaging& new_info);

            uint64_t get_page_size();

            uint64_t get_paging_info();
            void invalidate_addr(uint64_t addr);

        private:
            // Virtual address!
            pml4* paging_info; 
    };

    x86_64::paging::pml4* get_current_info();
    void set_current_info(x86_64::paging::pml4* info);


} // x86_64::paging


#endif