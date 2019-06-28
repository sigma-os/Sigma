#include <Sigma/mm/hmm.h>

void mm::hmm::init(){
    alloc::init();
    return;
}

void* mm::hmm::kmalloc(size_t size){
    return alloc::alloc(size);
}

void mm::hmm::kfree(void* ptr){
    alloc::free(ptr);
}

void* mm::hmm::realloc(void* ptr, size_t size){
   return alloc::realloc(ptr, size);
}