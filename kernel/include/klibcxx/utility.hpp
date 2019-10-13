#ifndef SIGMA_KERNEL_TYPES_PAIR
#define SIGMA_KERNEL_TYPES_PAIR

#include <klibcxx/common.hpp>
#include <type_traits>

namespace KLIBCXX_NAMESPACE_NAME
{
    template<typename T>
    constexpr typename std::remove_reference_t<T>::type&& move(T&& t) noexcept {
        return static_cast<typename std::remove_reference_t<T>::type&&>(t);
    }

    template<typename T>
    constexpr T&& forward(typename std::remove_reference_t<T>::type& t) noexcept {
        return static_cast<T&&>(t);
    }

    template<typename T>
    constexpr T&& forward(typename std::remove_reference_t<T>::type&& t) noexcept {
        static_assert(!std::is_lvalue_reference<T>::value);
        return static_cast<T&&>(t);
    }

    template<typename T1, typename T2>
    struct pair {
        using first_type = T1;
        using second_type = T2;

        constexpr pair() = default;

        constexpr pair(const T1& x, const T2& y): first(x), second(y) {}

        template<typename U1, typename U2>
        constexpr pair(U1&& x, U2&& y): first(std::forward<U1>(x)), second(std::forward<U2>(y)) {}

        template<typename U1, typename U2>
        constexpr pair(const pair<U1, U2> p): first(p.first), second(p.second) {}

        template<typename U1, typename U2>
        constexpr pair(const pair<U1, U2>& p): first(p.first), second(p.second) {}

        template<typename U1, typename U2>
        constexpr pair(pair<U1, U2>&& p): first(std::forward<U1>(p.first)), first(std::forward<U1>(p.first)) {}

        pair(const pair& p) = default;
        pair(pair&& p) = default;

        constexpr pair& operator=(const pair& other){
            this->first = other.first;
            this->second = other.second;
            return *this;
        }

        template<typename U1, typename U2>
        constexpr pair& operator=(const pair<U1, U2>& other){
            this->first = other.first;
            this->second = other.second;
            return *this;
        }

        constexpr pair& operator=(pair&& other){
            this->first = std::move(other.first);
            this->second = std::move(other.second);
            return *this;
        }

        template<class U1, class U2>
        constexpr pair& operator=(pair<U1,U2>&& other){
            this->first = std::forward<U1>(other.first);
            this->second = std::forward<U2>(other.second);
            return *this;
        }

        constexpr void swap(pair& other) noexcept {
            auto tmp = *this;
            *this = other;
            other = tmp;
        }

        //TODO: Comparison functions

        T1 first;
        T2 second;
    };
} // namespace types



#endif