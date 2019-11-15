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
    void init_simd();
    void save_state(proc::process::thread* thread);
    void restore_state(proc::process::thread* thread);
    void clone_state(uint8_t** old_thread, uint8_t** new_thread);
} // namespace proc::simd


#endif