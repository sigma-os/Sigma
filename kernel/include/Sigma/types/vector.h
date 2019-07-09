#ifndef SIGMA_KERNEL_TYPES_VECTOR
#define SIGMA_KERNEL_TYPES_VECTOR

#include <Sigma/common.h>
#include <klibc/stdlib.h>

namespace types
{
    template<typename T>
    class vector_iterator {
        public:
        explicit vector_iterator(T* entry): entry(entry) { }
        T& operator*(){
            return entry->item;
        }
        void operator++(){
            if(this->entry) this->entry++;
        }
        bool operator!=(vector_iterator& it){
            //if((this->entry->item == it.entry->item) && (this->entry->prev == it.entry->prev) && (this->entry->next == it.entry->next)) return false;
            if(this->entry == it.entry) return false;

            return true;
        }
        T* entry;
    };

    template<typename T>
    class vector {
        public:
        vector(): length(5), offset(0), data(reinterpret_cast<T*>(malloc(sizeof(T) * length))){
        }

        ~vector(){
            free(reinterpret_cast<void*>(this->data));
        }

        void push_back(T value){
            if((offset + 1) >= length) realloc(data, length + 5);
            data[offset++] = value;
        }

        T& operator[](size_t index){
            if(index >= length) PANIC("Tried to access vector out of bounds");

            return *data[index];
        }

        linked_list_iterator<T> begin(){
            return linked_list_iterator<T>(data);
        }

        linked_list_iterator<T> end(){
            return linked_list_iterator<T>(data + offset);
        }

        private:
        size_t length;
        size_t offset;
        T* data;
    };
} // namespace types


#endif