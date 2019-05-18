#include <Sigma/mm/hmm.h>

IPaging* paging;

void mm::hmm::init(IPaging& vmm){
    paging = &vmm;
    mm::slab::slab_init(vmm);
}

void* mm::hmm::kmalloc(size_t size){
    return mm::slab::slab_alloc(size);
}

void mm::hmm::kfree(void* ptr){
    mm::slab::slab_free(ptr);
}