#ifndef SIGMA_KLIBCXX_MUTEX
#define SIGMA_KLIBCXX_MUTEX

#include <klibcxx/common.hpp>

#include KLIBCXX_NATIVE_MUTEX_INCLUDE

namespace KLIBCXX_NAMESPACE_NAME {
    class mutex {
        public:
        constexpr mutex() noexcept : handle({}) {}

        ~mutex(){}

        mutex(const mutex&) = delete;
        mutex& operator =(const mutex&) = delete;

        void lock(){
            handle.lock();
        }

        bool try_lock(){
            return handle.try_lock();
        }

        void unlock(){
            handle.unlock();
        }

        using native_handle_type = KLIBCXX_NATIVE_MUTEX_TYPE;
        native_handle_type& native_handle(){
            return handle;
        }

        private:
        native_handle_type handle;
    };


    template<typename Mutex>
    class lock_guard {
        public:
        using mutex_type = Mutex;
        explicit lock_guard(mutex_type& m): _mutex(m){
            _mutex.lock();
        }

        ~lock_guard(){
            _mutex.unlock();
        }

        lock_guard& operator=(const lock_guard&) = delete;
        lock_guard(const lock_guard&) = delete;

        private:
        mutex_type& _mutex;
    };
}

#endif // !SIGMA_KLIBCXX_MUTEX