#include <Sigma/mm/hmm.h>

void mm::hmm::init(){
    mm::slab::slab_init();
}

void* mm::hmm::kmalloc(size_t size){
    return mm::slab::slab_alloc(size);
}

void mm::hmm::kfree(void* ptr){
    mm::slab::slab_free(ptr);
}