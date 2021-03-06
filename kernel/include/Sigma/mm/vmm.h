#ifndef SIGMA_KERNEL_MM_VMM
#define SIGMA_KERNEL_MM_VMM

#include <Sigma/common.h>
#include <Sigma/misc/misc.h>

#include <Sigma/arch/x86_64/paging.h>

namespace mm::vmm
{
    class kernel_vmm {
        public:
        static x86_64::paging::context& get_instance(){
            return _instance();
        }

        kernel_vmm(kernel_vmm const&) = delete;
        void operator =(kernel_vmm const&) = delete;

        private:

        static x86_64::paging::context& _instance();

        kernel_vmm() = default;
    };
} // mm::vmm




#endif