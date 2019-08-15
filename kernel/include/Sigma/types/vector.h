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
            return *entry;
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
        vector(): length(16), offset(0), data(reinterpret_cast<T*>(malloc(sizeof(T) * length))){
            data = new (malloc(sizeof(T) * length)) T;
        }

        ~vector(){
            free(static_cast<void*>(this->data));
        }

        void push_back(T value){
            if((offset + 1) >= length){
                //data = reinterpret_cast<T*>(realloc(data, sizeof(T) * (length * 2)));
                data = new (realloc(data, sizeof(T) * (length * 2))) T;
                length *= 2;
            } 
            data[offset++] = value;
        }

        [[nodiscard]]
        T& operator[](size_t index){
            if(index >= length) PANIC("Tried to access vector out of bounds");

            return data[index];
        }

        [[nodiscard]]
        vector_iterator<T> begin(){
            return vector_iterator<T>(data);
        }

        [[nodiscard]]
        vector_iterator<T> end(){
            return vector_iterator<T>(data + offset);
        }

        private:
        size_t length;
        size_t offset;
        T* data;
    };
} // namespace types


#endif