#pragma once

#include <stdint.h>

constexpr uint64_t pow2(uint64_t power){
    return 1 << power;
}

constexpr uint64_t div_ceil(uint64_t val, uint64_t div) {
	return (val + div - 1) / div;
}

constexpr uint64_t pow(uint64_t base, uint64_t pow){
    uint64_t tmp = 1;
    for(uint64_t i = 0; i < pow; i++)
        tmp *= base;
    return tmp;
}