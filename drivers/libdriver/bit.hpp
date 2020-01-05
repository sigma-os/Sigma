#pragma once

#include <stdint.h>
#include <memory>

#include <libdriver/math.hpp>

constexpr uint32_t bswap32(uint32_t dword) {
    return (uint32_t)((dword>>24) & 0xFF)
        | ((dword<<8) & 0xFF0000)
        | ((dword>>8)&0xFF00)
        | ((dword<<24)&0xFF000000);
}

template<uint64_t nBits>
class bitmap {
    public:
    constexpr bitmap(){
        n_bytes = div_ceil(nBits, 8);
        array = std::make_unique<uint8_t[]>(n_bytes);
    }

    constexpr void set(uint64_t bit){
        uint64_t index = bit / 8;
        uint64_t offset = bit % 8;

        array[index] |= (1ull << offset);
    }

    constexpr void clear(uint64_t bit){
        uint64_t index = bit / 8;
        uint64_t offset = bit % 8;

        array[index] &= ~(1ull << offset);
    }

    constexpr bool test(uint64_t bit){
        uint64_t index = bit / 8;
        uint64_t offset = bit % 8;

        bool state = (array[index] & (1ull << offset)) ? true : false;
        return state;
    }

    constexpr uint64_t get_free_bit(){
        for(size_t i = 0; i < n_bytes; i++){
            if(array[i] != 0xFF){ // Test if this entry is full
                for(uint64_t j = 0; j < 8; j++){
                    uint64_t bit = 1ull << j;
                    if(!(array[i] & bit))
                        return i * 8 + j;
                }
            }
        }

        return (uint64_t)-1;
    }

    private:
    size_t n_bytes;
    std::unique_ptr<uint8_t[]> array;
};