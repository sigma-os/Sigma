#ifndef SIGMA_KERNEL_ARCH_X86_64_PAGING
#define SIGMA_KERNEL_ARCH_X86_64_PAGING

#include <Sigma/common.h>
#include <Sigma/interfaces/paging_manager.h>

#include <Sigma/mm/pmm.h>

namespace x86_64::paging
{
    constexpr uint64_t paging_structures_n_entries = 512;


    struct pml4 {
        uint64_t entries[paging_structures_n_entries];
    } __attribute__((packed));

    struct pdpt {
        uint64_t entries[paging_structures_n_entries];
    } __attribute__((packed));

    struct pd {
        uint64_t entries[paging_structures_n_entries];
    } __attribute__((packed));

    struct pt {
        uint64_t entries[paging_structures_n_entries];
    } __attribute__((packed));

    constexpr uint64_t page_entry_present = 0;
    constexpr uint64_t page_entry_writeable = 1;
    constexpr uint64_t page_entry_user = 2;
    constexpr uint64_t page_entry_write_through = 3;
    constexpr uint64_t page_entry_cache_disable = 4;
    constexpr uint64_t page_entry_accessed = 5;
    constexpr uint64_t page_entry_dirty = 6;
    constexpr uint64_t page_entry_huge = 7;
    constexpr uint64_t page_entry_global = 8;
    constexpr uint64_t page_entry_no_exectute = 63;

    class paging : public virtual IPaging {
        public:
            paging(): paging_info(nullptr) {}
            void init();
            void deinit();

            bool map_page(uint64_t phys, uint64_t virt, uint64_t flags);

            void set_paging_info();

            void clone_paging_info(IPaging& new_info);

            uint64_t get_page_size();

            uint64_t get_paging_info();

        private:
            // Virtual address!
            pml4* paging_info; 
    };

    x86_64::paging::paging* get_current_info();
    void set_current_info(x86_64::paging::paging* info);


} // x86_64::paging


#endif