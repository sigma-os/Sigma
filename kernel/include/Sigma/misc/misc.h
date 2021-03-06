#ifndef SIGMA_MISC
#define SIGMA_MISC

#include <cstdint>
#include <cstddef>
#include <Sigma/misc/panic.h>
#include <type_traits>

namespace misc
{
	struct uuid {
		std::byte data[16];
	};

	class id_generator {
		public:
		constexpr id_generator(): next_id{0} {}

		uint64_t id(){
				return next_id++;
		}

		private:
		uint64_t next_id;
	};

	template<typename T>
	class lazy_initializer {
		public:
		template<typename... Args>
		void init(Args&&... args){
			if(this->_initialized) return;
			new (&_storage) T(std::forward<Args>(args)...);
			this->_initialized = true;
		}

		void deinit(){
			_initialized = false;
		}

		explicit operator bool(){
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
			PANIC("Tried to access uninitialized lazy variable");
			return nullptr; // Unreachable?
		}

		bool is_initialized(){
			return _initialized;
		}

		private:
		bool _initialized = false;
		std::aligned_storage_t<sizeof(T), alignof(T)> _storage;
	};

	namespace kernel_args {
		void init(char* str);

		bool get_bool(const char* key);
		const char* get_str(const char* key);
	}

	template <typename Enumeration>
	constexpr auto as_integer(Enumeration const value) -> typename std::underlying_type<Enumeration>::type
	{
		return static_cast<typename std::underlying_type<Enumeration>::type>(value);
	}

	constexpr uint64_t div_ceil(uint64_t val, uint64_t div) {
		return (val + div - 1) / div;
	}

	constexpr bool is_canonical(uint64_t addr){
		return ((addr <= 0x00007fffffffffff) || ((addr >= 0xffff800000000000) && (addr <= 0xffffffffffffffff)));
	}

	constexpr uint64_t pow(uint64_t base, uint64_t pow){
		uint64_t tmp = 1;
		for(uint64_t i = 0; i < pow; i++) tmp *= base;
		return tmp;
	}

	constexpr size_t min(size_t a, size_t b){
		return (a < b) ? a : b;
	}

	constexpr uint64_t compile_time_prng(uint64_t r, uint64_t seed, uint64_t iterations){
		/*
		 * Xn = rXn-1(1 - Xn-1)
		 * is chaotic for most values of r
		 * See: https://en.wikipedia.org/wiki/Logistic_map
		 *      https://www.youtube.com/watch?v=ovJcsL7vyrk
		 */

		auto iterate = [](uint64_t r, uint64_t x) -> uint64_t { return r * x * (1 - x); };

		uint64_t x = seed;

		for(uint64_t i = 0; i < iterations; i++)
			x = iterate(r, x);

		return x;
	}
} // namespace misc

#endif
