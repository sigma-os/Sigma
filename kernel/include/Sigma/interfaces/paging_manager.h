#ifndef SIGMA_KERNEL_INTERFACES_PAGING_MANAGER
#define SIGMA_KERNEL_INTERFACES_PAGING_MANAGER

#include <Sigma/common.h>

const uint64_t map_page_flags_present = (1 << 0);
const uint64_t map_page_flags_writable = (1 << 1);
const uint64_t map_page_flags_user = (1 << 2);
const uint64_t map_page_flags_no_execute = (1 << 3);
const uint64_t map_page_flags_cache_disable = (1 << 4);

class IPaging {
    public:
        virtual void init() = 0;
        virtual void deinit() = 0;

        virtual uint64_t get_page_size() = 0;

        virtual bool map_page(uint64_t phys, uint64_t virt, uint64_t flags) = 0;

        // Update the paging in the hardware, on x86_64 load cr3
        virtual void set_paging_info() = 0;

        virtual void clone_paging_info(IPaging& new_info) = 0;

    protected:
        ~IPaging() { }
};

#endif