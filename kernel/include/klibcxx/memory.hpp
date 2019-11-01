#ifndef SIGMA_KLIBCXX_MEMORY
#define SIGMA_KLIBCXX_MEMORY

#include <klibcxx/common.hpp>
#include <klibcxx/utility.hpp>
#include <klibcxx/functional.hpp>
#include <type_traits>

namespace KLIBCXX_NAMESPACE_NAME {
    template<typename T>
    struct default_delete {
        constexpr default_delete() noexcept = default;

        template<typename U>
        default_delete(const default_delete<U>& d){};

        void operator()(T* ptr) const{
            delete ptr;
        }
    };

    template<typename T>
    struct default_delete<T[]> {
        constexpr default_delete() noexcept = default;

        template<typename U>
        default_delete(const default_delete<U[]>& d){};

        template <typename U>
        void operator()(U* ptr) const{
            delete[] ptr;
        }
    };

    template<typename T, typename Deleter = KLIBCXX_NAMESPACE_NAME::default_delete<T>>
    class _unique_ptr_base {
        template <typename T_, typename Deleter_, typename = void>
        struct __pointer {
          using type = T_*;
        };

        template <typename T_, typename Deleter_>
        struct __pointer<T_, Deleter_, void_t<typename remove_reference<Deleter_>::type::pointer>> {
          using type = typename remove_reference<Deleter_>::type::pointer;
        };

        protected:
        using _element_type = T;
        using _deleter_type = Deleter;

        using _pointer = typename __pointer<_element_type, _deleter_type>::type;
    };

    template<typename T, typename Deleter = KLIBCXX_NAMESPACE_NAME::default_delete<T>>
    class unique_ptr : public _unique_ptr_base<T, Deleter> {
        public:
        using pointer = typename _unique_ptr_base<T, Deleter>::_pointer;

        using element_type = typename _unique_ptr_base<T, Deleter>::_element_type;
        using deleter_type = typename _unique_ptr_base<T, Deleter>::_deleter_type;

        constexpr unique_ptr() noexcept = default;
        constexpr unique_ptr(nullptr_t ptr) noexcept : _ptr() {}

        explicit unique_ptr(pointer p) noexcept : _ptr(p) {}

        unique_ptr(pointer p, typename KLIBCXX_NAMESPACE_NAME::conditional<KLIBCXX_NAMESPACE_NAME::is_reference<deleter_type>::value, \
                   deleter_type, const deleter_type&>::type d) noexcept : _ptr(p), _deleter(d) {}
        
        unique_ptr(pointer p, KLIBCXX_NAMESPACE_NAME::remove_reference_t<deleter_type>&& d) noexcept 
                : _ptr(KLIBCXX_NAMESPACE_NAME::move(p)), _deleter(KLIBCXX_NAMESPACE_NAME::move(d)) { 
            static_assert(!std::is_reference_v<deleter_type>); 
        }

        unique_ptr(unique_ptr&& u) noexcept : _ptr(u.release()), _deleter(KLIBCXX_NAMESPACE_NAME::forward<deleter_type>(u.get_deleter())) {}

        template<typename U, typename E>
        unique_ptr(unique_ptr<U, E>&& u) noexcept : _ptr(u.release()), _deleter(KLIBCXX_NAMESPACE_NAME::forward<E>(u.get_deleter())) {}

        ~unique_ptr(){
            if(this->get() == nullptr)
                return;
            
            this->get_deleter()(get());
        }        

        unique_ptr& operator=(unique_ptr&& r) noexcept {
            reset(r.release());
            get_deleter() = KLIBCXX_NAMESPACE_NAME::forward<deleter_type>(r.get_deleter());
            return *this;
        }

        template<typename U, typename E>
        unique_ptr& operator=(unique_ptr<U,E>&& r) noexcept {
            reset(r.release());
            get_deleter() = KLIBCXX_NAMESPACE_NAME::forward<deleter_type>(r.get_deleter());
            return *this;
        }	
        unique_ptr& operator=(nullptr_t) noexcept {
            reset();
            return *this;
        }

        explicit operator bool() const noexcept {
            return get() != nullptr;
        }

        

        typename std::add_lvalue_reference_t<T> operator*() const {
            return *get();
        }
        
        pointer operator->() const noexcept {
            return get();
        }

        pointer release() noexcept {
            pointer ret = this->_ptr;
            this->_ptr = pointer{};
            return ret;
        }

        void reset(pointer p = pointer{}) noexcept {
            auto* _p = this->_ptr;
            this->_ptr = p;
            if(_p != pointer{})
                this->get_deleter()(_p);
        }

        void swap(unique_ptr& other) noexcept {
            using KLIBCXX_NAMESPACE_NAME::swap;
            swap(_ptr, other._ptr); // ADL or something? You tell me
            swap(_deleter, other._deleter);
        }

        const deleter_type& get_deleter() const noexcept {
            return _deleter;
        }

        pointer get() const noexcept {
            return _ptr;
        }

        #pragma region Comparison

        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator==(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            return x.get() == y.get();
        }
        
        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator!=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            return x.get() != y.get();
        }

        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator<(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            using CT = std::common_type_t<typename unique_ptr<T1, D1>::pointer, typename unique_ptr<T2, D2>::pointer>;

            return std::less<CT>()(x.get(), y.get());
        }

        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator<=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            return !(y < x);
        }
    
        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator>(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            return y < x;
        }

        template<typename T1, typename D1, typename T2, typename D2>
        friend bool operator>=(const unique_ptr<T1, D1>& x, const unique_ptr<T2, D2>& y){
            return !(x < y);
        }

        template <typename T1, typename D1>
        friend bool operator==(const unique_ptr<T1, D1>& x, nullptr_t) noexcept{
            return !x;
        }

        template <typename T1, typename D1>
        friend bool operator==(nullptr_t, const unique_ptr<T1, D1>& x) noexcept{
            return !x;
        }

        template <typename T1, typename D1>
        friend bool operator!=(const unique_ptr<T1, D1>& x, nullptr_t) noexcept{
            return (bool)x;
        }

        template <typename T1, typename D1>
        friend bool operator!=(nullptr_t, const unique_ptr<T1, D1>& x) noexcept{
            return (bool)x;
        }

        template <typename T1, typename D1>
        friend bool operator<(const unique_ptr<T1, D1>& x, nullptr_t){
            return std::less<typename unique_ptr<T1, D1>::pointer>()(x.get(), nullptr);
        }

        template <typename T1, typename D1>
        friend bool operator<(nullptr_t, const unique_ptr<T1, D1>& y){
            return std::less<typename unique_ptr<T1, D1>::pointer>()(nullptr, y.get());
        }
	
        template <typename T1, typename D1>
        friend bool operator<=(const unique_ptr<T1, D1>& x, nullptr_t){
            return !(nullptr < x);
        }

        template <typename T1, typename D1>
        friend bool operator<=(nullptr_t, const unique_ptr<T1, D1>& y){
            return !(y < nullptr);
        }

        template <typename T1, typename D1>
        friend bool operator>(const unique_ptr<T1, D1>& x, nullptr_t){
            return nullptr < x;
        }

        template <typename T1, typename D1>
        friend bool operator>(nullptr_t, const unique_ptr<T1, D1>& y){
            return y < nullptr;
        }

        template <typename T1, typename D1>
        friend bool operator>=(const unique_ptr<T1, D1>& x, nullptr_t){
            return !(x < nullptr);
        }

        template <typename T1, typename D1>
        friend bool operator>=(nullptr_t, const unique_ptr<T1, D1>& y){
            return !(nullptr < y);
        }

        #pragma endregion

        private:
        pointer _ptr;
        Deleter _deleter;
    };

    template<typename T, typename Deleter>
    void swap(unique_ptr<T, Deleter>& x, unique_ptr<T, Deleter>& y) noexcept {
        x.swap(y);
    }

    template<typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args){
        return unique_ptr<T>(new T(KLIBCXX_NAMESPACE_NAME::forward<Args>(args)...));
    }
}

#endif // !SIGMA_KLIBCXX_MEMORY