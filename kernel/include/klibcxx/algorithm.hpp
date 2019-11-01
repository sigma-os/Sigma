#ifndef SIGMA_KLIBCXX_ALGORITHM
#define SIGMA_KLIBCXX_ALGORITHM

#include <klibcxx/common.hpp>
#include <klibcxx/utility.hpp>

namespace KLIBCXX_NAMESPACE_NAME {
    template<typename InputIt, typename UnaryFunction>
    constexpr UnaryFunction for_each(InputIt first, InputIt last, UnaryFunction f){
        for(auto it = first; it != last; ++it)
            f(*it);
        
        return std::move(f);
    }

    template<typename InputIt, typename UnaryPredicate>
    constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate p){
        for(auto it = first; it != last; ++it)
            if(p(*it))
                return it;
        
        return last;
    }
}

#endif // !SIGMA_KLIBCXX_ALGORITHM