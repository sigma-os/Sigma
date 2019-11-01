#ifndef SIGMA_KLIBCXX_FUNCTIONAL
#define SIGMA_KLIBCXX_FUNCTIONAL

#include <klibcxx/common.hpp>
#include <klibcxx/utility.hpp>

namespace KLIBCXX_NAMESPACE_NAME
{
    template<typename T = void>
    struct less;

    template<typename T>
    struct less {
        constexpr bool operator()(const T& lhs, const T& rhs) const {
            return lhs < rhs;
        }
    };

    template<>
    struct less<void> {
        template<typename T, typename U>
        constexpr auto operator()(T&& lhs, U&& rhs) const noexcept(noexcept(std::forward<T>(lhs) < std::forward<U>(rhs))) 
                    -> decltype(std::forward<T>(lhs) < std::forward<U>(rhs)) {
            return std::forward<T>(lhs) < std::forward<U>(rhs);
        }
    };
} // namespace KLIBCXX_NAMESPACE_NAME


#endif // !SIGMA_KLIBCXX_FUNCTIONAL