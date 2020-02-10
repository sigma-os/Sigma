#ifndef SIGMA_TYPES_BITMAP_H
#define SIGMA_TYPES_BITMAP_H

#include <Sigma/common.h>

namespace types
{
    class bitmap {
        public:
        bitmap() = default;
        bitmap(size_t n_bits){
            n_bytes = misc::div_ceil(n_bits, 8);
            array = new uint8_t[n_bytes];
        }

        ~bitmap(){
            delete[] array;
        }

        void set(uint64_t bit){
            uint64_t index = bit / 8;
            uint64_t offset = bit % 8;

            array[index] |= (1ull << offset);
        }

        void clear(uint64_t bit){
            uint64_t index = bit / 8;
            uint64_t offset = bit % 8;

            array[index] &= ~(1ull << offset);
        }

        bool test(uint64_t bit){
            uint64_t index = bit / 8;
            uint64_t offset = bit % 8;

            bool state = (array[index] & (1ull << offset)) ? true : false;
            return state;
        }

        uint64_t get_free_bit(){
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
        uint8_t* array;
    };
} // namespace types


#endif