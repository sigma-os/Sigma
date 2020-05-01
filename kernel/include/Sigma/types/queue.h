#ifndef SIGMA_TYPES_QUEUE_H
#define SIGMA_TYPES_QUEUE_H

#include <Sigma/common.h>
#include <klibcxx/utility.hpp>
#include <klibc/stdlib.h>
#include <klibc/stdio.h>
#include <Sigma/smp/cpu.h>

namespace types
{
    template<typename T>
    class queue
    {
    public:
        constexpr queue() noexcept : _lock{}, _length{0}, _offset{0}, _array{nullptr} {}
        ~queue(){
            std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
            std::lock_guard guard{this->_lock};
            if(_array){
                for(size_t i = 0; i < this->_offset; i++)
                    _array[i].~T();
                free(_array);
            }
        }

        queue(const queue&) = delete;
        queue& operator=(const queue& other) = delete;

        void push(T item){
            std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
            std::lock_guard guard{this->_lock};
            
            ensure_capacity(_offset + 1);
            new (&_array[_offset]) T{item};
            _offset++;
        }
    
        T pop(){
            std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
            std::lock_guard guard{this->_lock};
            if(_offset == 0)
                PANIC("Tried to pop from queue with 0 elements");

            _offset--;
            T ret = std::move(_array[_offset]);
            _array[_offset].~T();
            return ret;
        }

        T& back(){
            std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
            std::lock_guard guard{this->_lock};
            if(_offset == 0)
                PANIC("Tried to get back of queue with 0 elements");
            
            return _array[_offset - 1];
        }

        size_t length(){
            std::lock_guard irq_guard{smp::cpu::get_current_cpu()->irq_lock};
            std::lock_guard guard{this->_lock};
            return _offset;
        }
    private:
        void ensure_capacity(size_t c){
			if(c <= _length)
				return;
			
			size_t new_len = _length + c + 16;
			T* new_data = (T*)malloc(sizeof(T) * new_len);
			for(size_t i = 0; i < _length; i++)
				new (&new_data[i]) T{std::move(_array[i])};

			for(size_t i = 0; i < _offset; i++)
				_array[i].~T();
			free(_array);

			_array = new_data;
			_length = new_len;
		}

        std::mutex _lock;
        size_t _length;
        size_t _offset;
        T* _array;
    };    
} // namespace types



#endif // !SIGMA_TYPES_QUEUE_H