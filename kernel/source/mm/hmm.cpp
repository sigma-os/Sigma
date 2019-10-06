#include <Sigma/mm/hmm.h>

void mm::hmm::init(){
    alloc::init();
    return;
}

NODISCARD_ATTRIBUTE
void* mm::hmm::kmalloc(size_t size){
    return alloc::alloc(size);
}

NODISCARD_ATTRIBUTE
void* mm::hmm::kmalloc_a(size_t size, uint64_t align){
    return alloc::alloc_a(size, align);
}

void mm::hmm::kfree(void* ptr){
    alloc::free(ptr);
}

NODISCARD_ATTRIBUTE
void* mm::hmm::realloc(void* ptr, size_t size){
   return alloc::realloc(ptr, size);
}