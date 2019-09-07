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
} // namespace proc::simd


#endif