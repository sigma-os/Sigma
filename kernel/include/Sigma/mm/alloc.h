#ifndef SIGMA_KERNEL_ALLOC
#define SIGMA_KERNEL_ALLOC

#include <Sigma/common.h>
#include <Sigma/mm/pmm.h>
#include <Sigma/mm/vmm.h>
#include <Sigma/arch/x86_64/misc/spinlock.h>

// Simple heap allocator
namespace alloc
{
    constexpr uint16_t magic_low = 0xBEEF;
    constexpr uint16_t magic_high = 0xC0DE;

    struct alignas(64) header {
        uint16_t magic_low;
        struct header* next;
        size_t size;
        bool is_free;
        uint16_t magic_high;
        bool check_magic(){
            return (this->magic_low == alloc::magic_low) && (this->magic_high == alloc::magic_high);
        }

        void set_magic(){
            this->magic_low = alloc::magic_low;
            this->magic_high = alloc::magic_high;
        }
    };

    void init();
    void* alloc(size_t size);
    void* alloc_a(size_t size, uint64_t align);
    void* realloc(void* ptr, size_t size);
    void free(void* ptr);
    bool check_for_corruption(bool print_info);
} // namespace alloc


#endif