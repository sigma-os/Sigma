#ifndef SIGMA_PROC_SIMD
#define SIGMA_PROC_SIMD

#include <Sigma/common.h>

// Forward declaration
namespace proc::process
{
    struct thread;
} // namespace proc::process


namespace proc::simd
{
    struct PACKED_ATTRIBUTE fxsave_area {
        uint16_t fcw;
        uint16_t fsw;
        uint8_t ftw;
        uint8_t reserved;
        uint16_t fop;
        uint64_t fip;
        uint64_t fdp;
        uint32_t mxcsr;
        uint32_t mxcsr_mask;
        
        struct PACKED_ATTRIBUTE mm_entry {
            uint64_t low;
            uint16_t high;
            uint32_t res;
            uint16_t res_0;
        };
        static_assert(sizeof(mm_entry) == 16);
        mm_entry mm[8];
        
        struct PACKED_ATTRIBUTE xmm_entry {
            uint64_t low;
            uint64_t high;
        };
        static_assert(sizeof(xmm_entry) == 16);
        xmm_entry xmm[16];
        uint8_t reserved_0[48];
		uint8_t available[48];
    };
    static_assert(sizeof(fxsave_area) == 512);

    struct simd_state {
        simd_state(): data{nullptr} { init(); }
        void init();
        void deinit();

        simd_state(const simd_state&) = delete;
        simd_state(simd_state&&) = delete;
        simd_state& operator=(const simd_state& state);
        simd_state& operator=(simd_state&&) = delete;

        void save();
        void restore();
        private:

        uint8_t* data;
    };

    void init();
} // namespace proc::simd


#endif