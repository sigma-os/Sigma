#include <Sigma/mm/vmm.h>

misc::lazy_initializer<x86_64::paging::context> instance;

x86_64::paging::context& mm::vmm::kernel_vmm::_instance(){
    if(!instance){
        instance.init();
        instance->init();
    }

    return *instance;
}