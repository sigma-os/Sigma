#ifndef SIGMA_TYPES_QUEUE_H
#define SIGMA_TYPES_QUEUE_H

#include <Sigma/common.h>
#include <klibcxx/mutex.hpp>
#include <klibc/stdlib.h>

namespace types
{
    template<typename T>
    class queue
    {
    public:
        constexpr queue() noexcept : _length{0}, _offset{0}, _array{nullptr} {}
        ~queue(){
            if(_array)
                delete[] _array;
        }

        void reserve(size_t n){
            _length = _length + n;
            _array = static_cast<T*>(realloc(static_cast<void*>(_array), _length));
        }

        void push(T item){
            if(_offset + 1 >= _length)
                reserve(10); // Reserve 10 extra items

            _array[_offset++] = item;
        }

        T pop(){
            if(_offset == 0)
                PANIC("Tried to pop from queue with 0 elements");

            return _array[_offset--];
        }

        size_t length(){
            return _offset;
        }
    private:
        size_t _length;
        size_t _offset;
        T* _array;
    };    
} // namespace types



#endif // !SIGMA_TYPES_QUEUE_H