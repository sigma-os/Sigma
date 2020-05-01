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

    // Swap, for some reason is provided by freestanding stdlibc++
    /*template<typename T>
    constexpr void swap(T& a, T&b) noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
        T tmp = move(a);
        a = move(b);
        b = move(tmp);
    }*/

    template<typename T, typename U = T>
    constexpr T exchange(T& obj, U&& new_value){
        T ret = move(obj);
        obj = forward<U>(new_value);
        return ret;
    }

    template<typename T>
    constexpr std::add_const_t<T>& as_const(T& t) noexcept {
        return t;
    }

    template<typename T>
    void as_const(const T&&) = delete;

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
        constexpr pair(pair<U1, U2>&& p): first(std::forward<U1>(p.first)), second(std::forward<U2>(p.second)) {}

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

        constexpr void swap(pair& other) noexcept(
                                            std::is_nothrow_swappable_v<first_type> &&
                                            std::is_nothrow_swappable_v<second_type>
                                        ) {
            using KLIBCXX_NAMESPACE_NAME::swap;
            swap(first, other.first);
            swap(second, other.second);
        }

        //TODO: Comparison functions

        T1 first;
        T2 second;
    };

    template<typename T1, typename T2>
    constexpr std::pair<std::decay_t<T1>, std::decay_t<T2>> make_pair(T1&& t, T2&& u){
        return std::pair<std::decay_t<T1>, std::decay_t<T2>>{KLIBCXX_NAMESPACE_NAME::forward(t), KLIBCXX_NAMESPACE_NAME::forward(u)};
    }

    template<typename T1, typename T2>
    constexpr void swap(pair<T1, T2>& x, pair<T1, T2>& y) noexcept(noexcept(x.swap(y))) {
        x.swap(y);
    }

    template<typename T>
    struct tuple_size; // Forward declaration

    template<typename T1, typename T2>
    struct tuple_size<pair<T1, T2>> : std::integral_constant<std::size_t, 2> {};
} // namespace types



#endif