#ifndef SIGMA_KERNEL_TYPES_PAIR
#define SIGMA_KERNEL_TYPES_PAIR

#include <Sigma/common.h>

namespace types
{
    template<typename A, typename B>
    class pair {
        public:
        constexpr pair(A a, B b): a(a), b(b) {}
        pair() {}
        A a;
        B b; 
    };
} // namespace types



#endif