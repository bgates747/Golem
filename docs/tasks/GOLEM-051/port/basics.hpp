#pragma once
namespace std {
using size_t=decltype(sizeof(0));using ptrdiff_t=decltype((char*)0-(char*)0);
template<class T>class initializer_list {
    const T*data_;size_t size_;
    constexpr initializer_list(const T*p,size_t n):data_(p),size_(n){}
public:constexpr initializer_list():data_(nullptr),size_(0){}constexpr const T*begin()const{return data_;}constexpr const T*end()const{return data_+size_;}constexpr size_t size()const{return size_;}
};
template<bool C,class T=void>struct enable_if{};template<class T>struct enable_if<true,T>{using type=T;};
template<bool C,class T=void>using enable_if_t=typename enable_if<C,T>::type;
template<class T>struct remove_reference{using type=T;};template<class T>struct remove_reference<T&>{using type=T;};template<class T>struct remove_reference<T&&>{using type=T;};
template<class T>constexpr typename remove_reference<T>::type&&move(T&&v){return static_cast<typename remove_reference<T>::type&&>(v);}
template<class T>constexpr T&&forward(typename remove_reference<T>::type&v){return static_cast<T&&>(v);}
template<class T>constexpr T&&forward(typename remove_reference<T>::type&&v){return static_cast<T&&>(v);}
template<class T>struct is_integral{static constexpr bool value=false;};
#define AG_INTEGRAL(T) template<>struct is_integral<T>{static constexpr bool value=true;};
AG_INTEGRAL(bool) AG_INTEGRAL(char) AG_INTEGRAL(signed char) AG_INTEGRAL(unsigned char)
AG_INTEGRAL(short) AG_INTEGRAL(unsigned short) AG_INTEGRAL(int) AG_INTEGRAL(unsigned int)
AG_INTEGRAL(long) AG_INTEGRAL(unsigned long) AG_INTEGRAL(long long) AG_INTEGRAL(unsigned long long)
#undef AG_INTEGRAL
template<class T>struct is_signed{static constexpr bool value=T(-1)<T(0);};
class exception{public:virtual const char*what()const noexcept{return "exception";}virtual~exception()noexcept=default;};
}
inline void*operator new(std::size_t,void*p)noexcept{return p;}
inline void operator delete(void*,void*)noexcept{}
void*operator new(std::size_t);void operator delete(void*)noexcept;void operator delete(void*,std::size_t)noexcept;
void*operator new[](std::size_t);void operator delete[](void*)noexcept;void operator delete[](void*,std::size_t)noexcept;
