// Binary64 compiler calculations, independent of AgonDev's 32-bit double ABI.
#pragma once
extern "C" {
#include "softfloat.h"
}
namespace ag {
struct Real {
    float64_t raw;
    Real():raw{0}{}
    template<class T,class=std::enable_if_t<std::is_integral<T>::value>>Real(T v){
        if constexpr(std::is_signed<T>::value)raw=i64_to_f64((int64_t)v);else raw=ui64_to_f64((uint64_t)v);
    }
    Real(float v){float32_t f;memcpy(&f.v,&v,4);raw=f32_to_f64(f);}
    Real(double v){if constexpr(sizeof(double)==8)memcpy(&raw.v,&v,8);else{float32_t f;memcpy(&f.v,&v,4);raw=f32_to_f64(f);}}
    static Real bits(uint64_t bits){Real r;r.raw.v=bits;return r;}
    explicit operator float()const{auto f=f64_to_f32(raw);float v;memcpy(&v,&f.v,4);return v;}
    template<class T,class=std::enable_if_t<std::is_integral<T>::value>>explicit operator T()const{
        if constexpr(std::is_signed<T>::value)return (T)f64_to_i64_r_minMag(raw,false);else return (T)f64_to_ui64_r_minMag(raw,false);
    }
    Real operator-()const{return bits(raw.v^0x8000000000000000ull);}
    friend Real operator+(Real a,Real b){Real r;r.raw=f64_add(a.raw,b.raw);return r;}
    friend Real operator-(Real a,Real b){Real r;r.raw=f64_sub(a.raw,b.raw);return r;}
    friend Real operator*(Real a,Real b){Real r;r.raw=f64_mul(a.raw,b.raw);return r;}
    friend Real operator/(Real a,Real b){Real r;r.raw=f64_div(a.raw,b.raw);return r;}
    friend bool operator==(Real a,Real b){return f64_eq(a.raw,b.raw);}
    friend bool operator!=(Real a,Real b){return !f64_eq(a.raw,b.raw);}
    friend bool operator<(Real a,Real b){return f64_lt(a.raw,b.raw);}
    friend bool operator<=(Real a,Real b){return f64_le(a.raw,b.raw);}
    friend bool operator>(Real a,Real b){return b<a;}
    friend bool operator>=(Real a,Real b){return b<=a;}
};
Real parse_real(const char*,char**);
unsigned format_real(Real,unsigned,char*,unsigned);
}
