#ifndef SIGMA_KLIBCXX_MUTEX
#define SIGMA_KLIBCXX_MUTEX

#include <klibcxx/common.hpp>

namespace KLIBCXX_NAMESPACE_NAME {
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