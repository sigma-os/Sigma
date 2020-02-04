#ifndef SIGMA_GENERIC_EVENT_H
#define SIGMA_GENERIC_EVENT_H

#include <Sigma/common.h>
#include <atomic>

namespace events
{
    class event {
        public:
        event(): count{0} {}

        void trigger(){
            count.store(1);
        }

        bool has_triggered(){
            uint64_t val = 1;
            if(count.compare_exchange_strong(val, 0))
                return true;
            else
                return false;
        }

        private:
        std::atomic<size_t> count;
    };
} // namespace events




#endif