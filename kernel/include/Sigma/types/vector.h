#ifndef SIGMA_KERNEL_TYPES_VECTOR
#define SIGMA_KERNEL_TYPES_VECTOR

#include <Sigma/common.h>
#include <klibc/stdlib.h>
#include <klibcxx/utility.hpp>

namespace types
{
    template<typename T>
    class vector {
        public:
        using iterator = T*;
        using const_iterator = const T*;

        vector(): _length{16}, _offset{0} {
            _data = new (malloc(sizeof(T) * _length)) T;
        }

        ~vector(){
			for(auto&& entry : *this)
				entry.~T();
            free(static_cast<void*>(this->_data));
        }

		vector(const vector& b) {
			ensure_capacity(b._offset);
			for(size_t i = 0; i < b._offset; i++)
				new (&_data[i]) T{b._data[i]};
			_offset = b._offset;
		}

		friend void swap(vector& a, vector& b){
			using std::swap;
			swap(a._length, b._length);
			swap(a._offset, b._offset);
			swap(a._data, b._data);
		}

		vector& operator=(vector<T>& other) {
			swap(*this, other);
			return *this;
		}


		void resize(size_t size){
			ensure_capacity(size + 1);
			if(size < _length){
				for(size_t i = size; i < _length; i++)
					_data[i].~T();
			} else {
				for(size_t i = size; i < _length; i++)
					new (&_data[i]) T{};
			}

			_offset = size;
		}

        void push_back(T value){
            ensure_capacity(_offset + 1);
            _data[_offset++] = value;
        }

		NODISCARD_ATTRIBUTE
		T* empty_entry() {
			ensure_capacity(_offset + 1);
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
		void ensure_capacity(size_t c){
			if(c <= _length)
				return;
			
			size_t new_len = _length + c + 16;
			T* new_data = (T*)malloc(sizeof(T) * new_len);
			for(size_t i = 0; i < _length; i++)
				new (&new_data[i]) T{std::move(_data[i])};

			for(size_t i = 0; i < _length; i++)
				_data[i].~T();
			free(_data);

			_data = new_data;
			_length = new_len;
		}

        size_t _length;
        size_t _offset;
        T* _data;
    };
} // namespace types


#endif