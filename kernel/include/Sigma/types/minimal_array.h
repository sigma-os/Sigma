#ifndef SIGMA_TYPES_MINIMAL_ARRAY
#define SIGMA_TYPES_MINIMAL_ARRAY

#include <Sigma/common.h>
#include <Sigma/types/vector.h>

namespace types
{
    // Array that holds NStaticElements statically and dynamically allocates more if needed
    template<int NStaticElements, typename T>
    class minimal_array
    {
    public:
        minimal_array(): current_index(0) {}
        
        NODISCARD_ATTRIBUTE
        T& at(int index){
            if(index < NStaticElements) 
                return static_elements[index];
            else {
                if(!dynamic_elements)
                    dynamic_elements.init();
                return dynamic_elements[index - NStaticElements];
            }    
        }

        NODISCARD_ATTRIBUTE
        T& empty_entry(){
            if(current_index < NStaticElements) 
                return static_elements[current_index++];
            else {
                if(!dynamic_elements)
                    dynamic_elements.init();
                
                auto& ret = *dynamic_elements->empty_entry();
                current_index++;
                return ret;
            }
        }
    private:
        int current_index;
        T static_elements[NStaticElements];
        misc::lazy_initializer<types::vector<T>> dynamic_elements;
    };
    
} // namespace types



#endif