#include <Sigma/mm/vmm.h>

misc::lazy_initializer<x86_64::paging::paging> instance;

x86_64::paging::paging& mm::vmm::kernel_vmm::_instance(){
    if(!instance){
        instance.init();
        instance->init();
    }

    return *instance;
}