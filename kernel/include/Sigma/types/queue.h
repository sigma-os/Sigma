#ifndef SIGMA_TYPES_QUEUE_H
#define SIGMA_TYPES_QUEUE_H

#include <Sigma/common.h>
#include <klibcxx/mutex.hpp>
#include <klibcxx/utility.hpp>
#include <klibc/stdlib.h>
#include <klibc/stdio.h>

namespace types
{
    template<typename T>
    class queue
    {
    public:
        constexpr queue() noexcept : _lock{}, _length{0}, _offset{0}, _array{nullptr} {}
        ~queue(){
            if(_array)
                delete[] _array;
        }

        queue(const queue&) = delete;
        queue& operator=(const queue& other) = delete;

        void push(T item){
            std::lock_guard guard{this->_lock};
            if((_offset + 1) >= _length)
                reserve(10); // Reserve 10 extra items

            new (&_array[_offset++]) T{item};
        }

        T pop(){
            std::lock_guard guard{this->_lock};
            if(_offset == 0)
                PANIC("Tried to pop from queue with 0 elements");

            _offset--;
            T ret = std::move(_array[_offset]);
            _array[_offset].~T();
            return ret;
        }

        NODISCARD_ATTRIBUTE
        T& back(){
            std::lock_guard guard{this->_lock};
            if(_offset == 0)
                PANIC("Tried to get back of queue with 0 elements");
            
            return _array[_offset - 1];
        }

        size_t length(){
            return _offset;
        }
    private:
        void reserve(size_t n){
            _length += n;
            _array = new (realloc(_array, _length * sizeof(T))) T;
        }

        std::mutex _lock;
        size_t _length;
        size_t _offset;
        T* _array;
    };    
} // namespace types



#endif // !SIGMA_TYPES_QUEUE_H