#ifndef SIGMA_MISC
#define SIGMA_MISC

#include <Sigma/common.h>
#include <type_traits>

namespace misc
{
    template<typename T>
    class lazy_initializer {
        public:
        template<typename... Args>
        void init(Args&&... args){
            if(this->_initialized) return;
            new (&_storage) T(args...);
            this->_initialized = true;
        }

        void deinit(){
            _initialized = false;
        }

        operator bool(){
            return this->_initialized;
        }
    
        T* operator ->(){
		    return get();
	    }
	    
        T& operator *(){
		    return *get();
	    }

        T* get(){
		    if(_initialized){
                return reinterpret_cast<T *>(&_storage);
            }
            PANIC("Tried to access unintialized lazy variable");
            return nullptr; // Unreachable?
	    }

        private:
        bool _initialized = false;
        std::aligned_storage_t<sizeof(T), alignof(T)> _storage;
    };

    template <typename Enumeration>
    constexpr auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type
    {
        return static_cast<typename std::underlying_type<Enumeration>::type>(value);
    }
} // namespace misc

#endif