#ifndef SIGMA_KERNEL_TYPES_VECTOR
#define SIGMA_KERNEL_TYPES_VECTOR

#include <Sigma/common.h>
#include <klibc/stdlib.h>

namespace types
{
    template<typename T>
    class vector {
        public:
        using iterator = T*;
        using const_iterator = const T*;

        vector(): _length(16), _offset(0){
            _data = new (malloc(sizeof(T) * _length)) T;
        }

        ~vector(){
            free(static_cast<void*>(this->_data));
        }

        void push_back(T value){
            if((_offset + 1) >= _length){
                _length *= 2;
                _data = new (realloc(_data, sizeof(T) * _length)) T;
                
            } 
            _data[_offset++] = value;
        }

		NODISCARD_ATTRIBUTE
		T* empty_entry() {
			if((_offset + 1) >= _length) {
				_length *= 2;
				_data = new(realloc(_data, sizeof(T) * _length)) T;
			}
			return new (&_data[_offset++]) T();
		}

		NODISCARD_ATTRIBUTE
		T& operator[](size_t index) {
			if(index >= _offset)
				PANIC("Tried to access vector out of bounds");

            return _data[index];
        }

		T* data(){
			return this->_data;
		}

		size_t size(){
			return _offset;
		}

		NODISCARD_ATTRIBUTE
		iterator begin() {
			return _data;
		}

		NODISCARD_ATTRIBUTE
		iterator end() {
			return _data + _offset;
		}

		NODISCARD_ATTRIBUTE
		T& back(){
			return *(_data + _offset - 1);
		}

        NODISCARD_ATTRIBUTE
		const_iterator begin() const {
			return _data;
		}

		NODISCARD_ATTRIBUTE
		const_iterator end() const {
			return _data + _offset;
		}

		NODISCARD_ATTRIBUTE
		T& back() const {
			return *(_data + _offset - 1);
		}


        private:
        size_t _length;
        size_t _offset;
        T* _data;
    };
} // namespace types


#endif