#ifndef SIGMA_KERNEL_MM_SLAB
#define SIGMA_KERNEL_MM_SLAB

#include <Sigma/common.h>

#include <Sigma/interfaces/paging_manager.h>
#include <Sigma/mm/pmm.h>

#include <Sigma/arch/x86_64/misc/spinlock.h>

namespace mm::slab
{
    struct slab_entry {
        slab_entry* next;
    };

    struct slab {
        public:
        slab* next_slab;
        slab_entry* free_list;
        uint64_t slab_start;
        uint64_t size;

        void init(uint64_t size);
        bool alloc(size_t sz, uint64_t& loc);
        bool free(uint64_t location);

        private:
        x86_64::spinlock::mutex slab_mutex;
    };

    void slab_init(IPaging& vmm);
    void* slab_alloc(size_t size);
    void slab_free(void* ptr);
} // mm::slab



#endif