#include <Sigma/mm/vmm.h>

misc::lazy_initializer<mm::vmm::manager<x86_64::paging::paging>> instance;

mm::vmm::manager<x86_64::paging::paging>& mm::vmm::kernel_vmm::_instance(){

    if(!instance){
        instance.init();
    }

    return *instance;
}