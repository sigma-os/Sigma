#ifndef SIGMA_KERNEL_MM_VMM
#define SIGMA_KERNEL_MM_VMM

#include <Sigma/common.h>
#include <Sigma/misc.h>
#include <Sigma/interfaces/paging_manager.h>

#include <Sigma/arch/x86_64/paging.h>

namespace mm::vmm
{
    template<typename T>
    class manager
    {
        public:
            manager(): item(T()), current_info(&item) {
                this->current_info->init();
            }
            ~manager(){
                this->current_info->deinit();
            }

            bool map_page(uint64_t phys, uint64_t virt, uint64_t flags){
                return this->current_info->map_page(phys, virt, flags);
            }

            void set(){
                this->current_info->set_paging_info();
            }

            void clone_info(T& new_info){
                this->current_info->clone_paging_info(new_info);
            }

            IPaging& get_paging_provider(){
                return *(this->current_info);
            }

            

        private:
            T item;
            IPaging* current_info;
    };


    class kernel_vmm {
        public:
        static mm::vmm::manager<x86_64::paging::paging>& get_instance(){
            return _instance();
        }

        kernel_vmm(kernel_vmm const&) = delete;
        void operator =(kernel_vmm const&) = delete;

        private:

        static mm::vmm::manager<x86_64::paging::paging>& _instance();

        kernel_vmm() {}

    };
} // mm::vmm




#endif