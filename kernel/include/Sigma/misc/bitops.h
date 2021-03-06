#ifndef SIGMA_KERNEL_BITOPS
#define SIGMA_KERNEL_BITOPS

#include <Sigma/common.h>

template<typename T>
class bitops {
public:
    static constexpr bool bit_set(T* item, uint64_t bit){
        T set = *item & (1ULL << bit);
        *item |= (1ULL << bit);
        return !(set == 0);
    }

    static constexpr bool bit_clear(T* item, uint64_t bit){
        T set = *item & (1ULL << bit);
        *item &= ~(1ULL << bit);
        return !(set == 0);
    }

    static constexpr bool bit_test(T* item, uint64_t bit){
        T set = *item & (1ULL << bit);
        return !(set == 0);
    }


    static constexpr bool bit_set(T& item, uint64_t bit){
        T set = item & (1ULL << bit);
        item |= (1ULL << bit);
        return !(set == 0);
    }

    static constexpr bool bit_clear(T& item, uint64_t bit){
        T set = item & (1ULL << bit);
        item &= ~(1ULL << bit);
        return !(set == 0);
    }

    static constexpr bool bit_test(T& item, uint64_t bit){
        T set = item & (1ULL << bit);
        return !(set == 0);
    }
};

#endif