#ifndef SIGMA_GENERIC_EVENT_H
#define SIGMA_GENERIC_EVENT_H

#include <Sigma/common.h>
#include <atomic>

namespace generic
{
    class event {
        public:
        event(): count{0} {}

        void trigger(){
            count.fetch_add(1);
        }

        bool has_triggered(){
            uint64_t val = 0;
            if(count.compare_exchange_strong(val, 0))
                return false;

            count.fetch_sub(1);
            return true;
        }

        private:
        std::atomic<size_t> count;
    };
} // namespace generic




#endif