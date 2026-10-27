// Purpose-built library surface for the existing Golem compiler on AgonDev.
// Copyright (c) 2026 Brandon R. Gates. Not a general C++ standard library.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <setjmp.h>
#include "basics.hpp"
#include "real.hpp"
#undef fpclassify
#undef isfinite
#ifndef FP_NAN
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
#endif
#ifdef AGONDEV
#include <agon/mos.h>
#else
#include <sys/stat.h>
#endif

namespace ag {
[[noreturn]] void panic(const char*);
extern const char*phase;
void*memory(size_t);
struct Cleanup {
    Cleanup *next=nullptr;
    void *object=nullptr;void (*destroy)(void*)=nullptr;
    void attach(void*,void(*)(void*));void detach();
    ~Cleanup(){detach();}
};
}
namespace std {
using ::size_t;using ::ptrdiff_t;using ::memcpy;using ::memset;
using ::tolower;using ::isspace;using ::isalpha;
template<class T> const T& min(const T&a,const T&b){return b<a?b:a;}
template<class T> const T& max(const T&a,const T&b){return a<b?b:a;}
template<class I,class F> bool all_of(I a,I b,F f){for(;a!=b;++a)if(!f(*a))return false;return true;}
template<class I,class F> bool any_of(I a,I b,F f){for(;a!=b;++a)if(f(*a))return true;return false;}
template<class I,class J> bool equal(I a,I b,J c){for(;a!=b;++a,++c)if(!(*a==*c))return false;return true;}
template<class I,class J> J copy(I a,I b,J c){for(;a!=b;++a,++c)*c=*a;return c;}
template<class A,class B> struct pair {
    A first;B second;
    pair():first(),second(){}
    pair(const A&a,const B&b):first(a),second(b){}
    pair(const A&a,B&&b):first(a),second(move(b)){}
    template<class C,class D>pair(const pair<C,D>&p):first(p.first),second(p.second){}
    bool operator==(const pair&p)const{return first==p.first&&second==p.second;}
};

class string {
    struct Storage {size_t refs,capacity;char bytes[1];};
    Storage*p_=nullptr;size_t size_=0;ag::Cleanup cleanup_;
    static void destroy(void*p){static_cast<string*>(p)->~string();}
    void attach(){cleanup_.attach(this,destroy);}
    void release(){if(p_&&!--p_->refs)free(p_);p_=nullptr;size_=0;}
    void ensure(size_t count){
        if(p_&&p_->refs==1&&p_->capacity>=count)return;
        size_t cap=p_?p_->capacity:15;while(cap<count)cap=cap*2+1;
#ifdef AGONDEV
        if(cap>0x70000ul)ag::panic("string exceeds available Agon memory");
#endif
        Storage*p=(Storage*)ag::memory(sizeof(Storage)+cap);
        if(!p)ag::panic("out of compiler memory");p->refs=1;p->capacity=cap;
        if(size_)memcpy(p->bytes,p_->bytes,size_);p->bytes[size_]=0;
        if(p_&&!--p_->refs)free(p_);p_=p;
    }
public:
    static constexpr size_t npos=(size_t)-1;
    string(){attach();}
    string(const char*s):string(){if(s){size_t n=strlen(s);ensure(n);memcpy(p_->bytes,s,n+1);size_=n;}}
    string(const char*s,size_t n):string(){ensure(n);memcpy(p_->bytes,s,n);size_=n;p_->bytes[n]=0;}
    string(size_t n,char ch):string(){ensure(n);memset(p_->bytes,ch,n);size_=n;p_->bytes[n]=0;}
    template<class I,class=enable_if_t<!is_integral<I>::value>>string(I a,I b):string(){for(;a!=b;++a)push_back((char)*a);}
    string(const string&s):p_(s.p_),size_(s.size_){if(p_)++p_->refs;attach();}
    string(string&&s)noexcept:p_(s.p_),size_(s.size_){s.p_=nullptr;s.size_=0;attach();}
    ~string(){cleanup_.detach();release();}
    string&operator=(const string&s){if(this!=&s){release();p_=s.p_;size_=s.size_;if(p_)++p_->refs;}return *this;}
    string&operator=(string&&s)noexcept{if(this!=&s){release();p_=s.p_;size_=s.size_;s.p_=nullptr;s.size_=0;}return *this;}
    string&operator=(const char*s){return *this=string(s);}
    size_t size()const{return size_;}size_t length()const{return size_;}bool empty()const{return !size_;}
    const char*data()const{return p_?p_->bytes:"";}const char*c_str()const{return data();}
    char*data(){ensure(size_);return p_->bytes;}
    const char*begin()const{return data();}const char*end()const{return data()+size_;}
    char*begin(){return data();}char*end(){return data()+size_;}
    const char&operator[](size_t n)const{return data()[n];}
    char&operator[](size_t n){ensure(size_);return p_->bytes[n];}
    char front()const{return data()[0];}char back()const{return data()[size_-1];}
    void clear(){release();}void reserve(size_t n){ensure(n);}
    void push_back(char ch){ensure(size_+1);p_->bytes[size_++]=ch;p_->bytes[size_]=0;}
    void pop_back(){if(size_){ensure(size_);p_->bytes[--size_]=0;}}
    string&operator+=(const string&s){if(&s==this){string cp(s);return *this+=cp;}size_t n=s.size_;ensure(size_+n);memcpy(p_->bytes+size_,s.data(),n);size_+=n;p_->bytes[size_]=0;return *this;}
    string&operator+=(const char*s){return *this+=string(s);}string&operator+=(char ch){push_back(ch);return *this;}
    string substr(size_t at,size_t n=npos)const{if(at>size_)ag::panic("string offset");return string(data()+at,min(n,size_-at));}
    size_t find(char ch,size_t at=0)const{for(;at<size_;++at)if(data()[at]==ch)return at;return npos;}
    size_t find(const string&s,size_t at=0)const{for(;at+s.size_<=size_;++at)if(!memcmp(data()+at,s.data(),s.size_))return at;return npos;}
    size_t rfind(const string&s,size_t at=npos)const{if(s.size_>size_)return npos;at=min(at,size_-s.size_);for(;;){if(!memcmp(data()+at,s.data(),s.size_))return at;if(!at--)break;}return npos;}
    int compare(size_t at,size_t n,const string&s)const{auto t=substr(at,n);return t==s?0:t<s?-1:1;}
    string&replace(size_t at,size_t n,const string&s){string r=substr(0,at);r+=s;r+=substr(at+min(n,size_-at));return *this=move(r);}
    bool operator==(const string&s)const{return size_==s.size_&&!memcmp(data(),s.data(),size_);}
    bool operator!=(const string&s)const{return !(*this==s);}
    bool operator<(const string&s)const{int c=memcmp(data(),s.data(),min(size_,s.size_));return c<0||(!c&&size_<s.size_);}
};
inline string operator+(string a,const string&b){a+=b;return a;}
inline string operator+(string a,char b){a+=b;return a;}
inline string operator+(const char*a,const string&b){string r(a);r+=b;return r;}
inline bool operator==(const char*a,const string&b){return b==string(a);}
inline bool operator!=(const char*a,const string&b){return !(a==b);}

class runtime_error:public exception {
    string text_;
public:runtime_error(const string&s):text_(s){}const char*what()const noexcept override{return text_.c_str();}
};
class logic_error:public runtime_error{public:using runtime_error::runtime_error;};
template<class T>string to_string(T value){char text[32];char*end=text+sizeof(text);char*p=end;bool neg=false;uint64_t n;
    if constexpr(is_signed<T>::value){neg=value<0;n=neg?uint64_t(-(value+1))+1:uint64_t(value);}else n=value;
    do{*--p=(char)('0'+n%10);n/=10;}while(n);if(neg)*--p='-';return string(p,(size_t)(end-p));}

template<class T>class vector {
    T*data_=nullptr;size_t size_=0,capacity_=0;ag::Cleanup cleanup_;
    static void destroy(void*p){static_cast<vector*>(p)->~vector();}
    void attach(){cleanup_.attach(this,destroy);}
public:
    using iterator=T*;using const_iterator=const T*;
    vector(){attach();}explicit vector(size_t n):vector(){resize(n);}
    vector(size_t n,const T&v):vector(){reserve(n);for(size_t i=0;i<n;++i)push_back(v);}
    vector(initializer_list<T> l):vector(){reserve(l.size());for(const auto&v:l)push_back(v);}
    template<class I,class=enable_if_t<!is_integral<I>::value>>vector(I a,I b):vector(){for(;a!=b;++a)push_back(*a);}
    vector(const vector&v):vector(){reserve(v.size_);for(const auto&x:v)push_back(x);}
    vector(vector&&v)noexcept:data_(v.data_),size_(v.size_),capacity_(v.capacity_){v.data_=nullptr;v.size_=v.capacity_=0;attach();}
    ~vector(){cleanup_.detach();clear();free(data_);}
    vector&operator=(const vector&v){if(this!=&v){clear();reserve(v.size_);for(const auto&x:v)push_back(x);}return *this;}
    vector&operator=(vector&&v)noexcept{if(this!=&v){clear();free(data_);data_=v.data_;size_=v.size_;capacity_=v.capacity_;v.data_=nullptr;v.size_=v.capacity_=0;}return *this;}
    vector&operator=(initializer_list<T>l){clear();for(const auto&v:l)push_back(v);return *this;}
    size_t size()const{return size_;}bool empty()const{return !size_;}
    T*data(){return data_;}const T*data()const{return data_;}
    T*begin(){return data_;}const T*begin()const{return data_;}T*end(){return size_?data_+size_:data_;}const T*end()const{return size_?data_+size_:data_;}
    T&operator[](size_t i){return data_[i];}const T&operator[](size_t i)const{return data_[i];}
    T&at(size_t i){if(i>=size_)ag::panic("vector offset");return data_[i];}const T&at(size_t i)const{if(i>=size_)ag::panic("vector offset");return data_[i];}
    T&front(){return data_[0];}const T&front()const{return data_[0];}T&back(){return data_[size_-1];}const T&back()const{return data_[size_-1];}
    void reserve(size_t n){if(n<=capacity_)return;size_t cap=capacity_?capacity_*2:4;if(cap<n)cap=n;
#ifdef AGONDEV
        if(cap>0x70000ul/sizeof(T))ag::panic("container exceeds available Agon memory");
#endif
        T*p=(T*)ag::memory(sizeof(T)*cap);if(!p)ag::panic("out of compiler memory");
        for(size_t i=0;i<size_;++i){new(p+i)T(move(data_[i]));data_[i].~T();}free(data_);data_=p;capacity_=cap;}
    void push_back(const T&v){if(size_==capacity_){T cp(v);reserve(size_+1);new(data_+size_)T(move(cp));}else new(data_+size_)T(v);++size_;}
    void push_back(T&&v){if(size_==capacity_){T cp(move(v));reserve(size_+1);new(data_+size_)T(move(cp));}else new(data_+size_)T(move(v));++size_;}
    void pop_back(){if(size_)data_[--size_].~T();}
    void clear(){while(size_)pop_back();}
    void resize(size_t n){reserve(n);while(size_<n){new(data_+size_)T();++size_;}while(size_>n)pop_back();}
    void resize(size_t n,const T&v){reserve(n);while(size_<n){new(data_+size_)T(v);++size_;}while(size_>n)pop_back();}
    template<class I>void assign(I a,I b){clear();for(;a!=b;++a)push_back(*a);}
    template<class I>T*insert(T*pos,I a,I b){size_t off=size_?(size_t)(pos-data_):0;if(off!=size_)ag::panic("non-tail vector insertion");for(;a!=b;++a)push_back(*a);return data_+off;}
    T*insert(T*pos,initializer_list<T>l){return insert(pos,l.begin(),l.end());}
    bool operator==(const vector&v)const{return size_==v.size_&&equal(begin(),end(),v.begin());}
    bool operator!=(const vector&v)const{return !(*this==v);}
};

template<class K,class V>class map {
    struct Node {pair<const K,V> value;Node*next;Node(const K&k,const V&v,Node*n):value(k,v),next(n){}};
    Node*head_=nullptr;size_t size_=0;vector<Node*>index_;ag::Cleanup cleanup_;
    static void destroy(void*p){static_cast<map*>(p)->~map();}
    void attach(){cleanup_.attach(this,destroy);}
    size_t lower(const K&k)const{size_t a=0,b=size_;while(a<b){size_t mid=a+(b-a)/2;if(index_[mid]->value.first<k)a=mid+1;else b=mid;}return a;}
public:
    struct iterator {Node*p=nullptr;pair<const K,V>&operator*()const{return p->value;}pair<const K,V>*operator->()const{return &p->value;}iterator&operator++(){p=p->next;return *this;}bool operator==(iterator i)const{return p==i.p;}bool operator!=(iterator i)const{return p!=i.p;}};
    map(){attach();}map(initializer_list<pair<const K,V>>l):map(){for(const auto&p:l)emplace(p.first,p.second);}
    template<class I>map(I a,I b):map(){for(;a!=b;++a)emplace(a->first,a->second);}
    map(const map&m):map(){for(const auto&p:m)emplace(p.first,p.second);}
    map(map&&m)noexcept:head_(m.head_),size_(m.size_),index_(move(m.index_)){m.head_=nullptr;m.size_=0;attach();}
    ~map(){cleanup_.detach();clear();}
    map&operator=(const map&m){if(this!=&m){clear();for(const auto&p:m)emplace(p.first,p.second);}return *this;}
    map&operator=(map&&m)noexcept{if(this!=&m){clear();head_=m.head_;size_=m.size_;index_=move(m.index_);m.head_=nullptr;m.size_=0;}return *this;}
    iterator begin()const{return {head_};}iterator end()const{return {};}
    size_t size()const{return size_;}bool empty()const{return !size_;}
    iterator find(const K&k)const{size_t at=lower(k);if(at<size_&&!(k<index_[at]->value.first))return {index_[at]};return {};}
    size_t count(const K&k)const{return find(k)!=end();}
    pair<iterator,bool>emplace(const K&k,const V&v){size_t at=lower(k);if(at<size_&&!(k<index_[at]->value.first))return {{index_[at]},false};Node*p=new Node(k,v,at<size_?index_[at]:nullptr);if(at)index_[at-1]->next=p;else head_=p;index_.push_back(p);for(size_t i=size_;i>at;--i)index_[i]=index_[i-1];index_[at]=p;++size_;return {{p},true};}
    pair<iterator,bool>emplace(const pair<const K,V>&p){return emplace(p.first,p.second);}
    V&operator[](const K&k){return emplace(k,V()).first->second;}
    V&at(const K&k){auto i=find(k);if(i==end())ag::panic("missing map key");return i->second;}
    const V&at(const K&k)const{auto i=find(k);if(i==end())ag::panic("missing map key");return i->second;}
    void erase(const K&k){size_t at=lower(k);if(at==size_||k<index_[at]->value.first)return;Node*p=index_[at];if(at)index_[at-1]->next=p->next;else head_=p->next;for(size_t i=at+1;i<size_;++i)index_[i-1]=index_[i];index_.pop_back();--size_;delete p;}
    void erase(iterator i){erase(i->first);}
    void clear(){while(head_){Node*p=head_;head_=p->next;delete p;}size_=0;index_.clear();}
};
template<class K>class set {
    map<K,char>items_;
public:
    struct iterator {typename map<K,char>::iterator i;const K&operator*()const{return i->first;}const K*operator->()const{return &i->first;}iterator&operator++(){++i;return *this;}bool operator==(iterator j)const{return i==j.i;}bool operator!=(iterator j)const{return i!=j.i;}};
    set()=default;set(initializer_list<K>l){for(const auto&v:l)insert(v);}
    template<class I>set(I a,I b){insert(a,b);}
    pair<iterator,bool>insert(const K&k){auto r=items_.emplace(k,0);return {{r.first},r.second};}
    template<class I>void insert(I a,I b){for(;a!=b;++a)insert(*a);}
    iterator begin()const{return {items_.begin()};}iterator end()const{return {items_.end()};}
    size_t count(const K&k)const{return items_.count(k);}size_t size()const{return items_.size();}bool empty()const{return items_.empty();}void erase(const K&k){items_.erase(k);}
};
template<class T,size_t N>struct array {T values[N];T&operator[](size_t i){return values[i];}const T&operator[](size_t i)const{return values[i];}T*begin(){return values;}const T*begin()const{return values;}T*end(){return values+N;}const T*end()const{return values+N;}constexpr size_t size()const{return N;}};

template<class T>class shared_ptr {
    struct Control {size_t refs;T value;Control():refs(1),value(){}};Control*p_=nullptr;ag::Cleanup cleanup_;
    static void destroy(void*p){static_cast<shared_ptr*>(p)->~shared_ptr();}void attach(){cleanup_.attach(this,destroy);}void release(){if(p_&&!--p_->refs)delete p_;p_=nullptr;}
public:
    shared_ptr(){attach();}shared_ptr(const shared_ptr&p):p_(p.p_){if(p_)++p_->refs;attach();}shared_ptr(shared_ptr&&p)noexcept:p_(p.p_){p.p_=nullptr;attach();}
    ~shared_ptr(){cleanup_.detach();release();}
    shared_ptr&operator=(const shared_ptr&p){if(this!=&p){release();p_=p.p_;if(p_)++p_->refs;}return *this;}shared_ptr&operator=(shared_ptr&&p)noexcept{if(this!=&p){release();p_=p.p_;p.p_=nullptr;}return *this;}
    T*operator->()const{return &p_->value;}T&operator*()const{return p_->value;}explicit operator bool()const{return p_!=nullptr;}void reset(){release();}
    static shared_ptr make(){shared_ptr r;r.p_=new Control();return r;}
};
template<class T>shared_ptr<T>make_shared(){return shared_ptr<T>::make();}
template<class T>class optional {
    alignas(T)unsigned char storage_[sizeof(T)];bool valid_=false;
    T*ptr(){return reinterpret_cast<T*>(storage_);}const T*ptr()const{return reinterpret_cast<const T*>(storage_);}
public:
    optional()=default;optional(const T&t){new(ptr())T(t);valid_=true;}
    optional(const optional&o){if(o){new(ptr())T(*o);valid_=true;}}
    optional(optional&&o){if(o){new(ptr())T(move(*o));valid_=true;}}
    ~optional(){reset();}void reset(){if(valid_){ptr()->~T();valid_=false;}}
    optional&operator=(const T&t){if(valid_)*ptr()=t;else{new(ptr())T(t);valid_=true;}return *this;}
    optional&operator=(const optional&o){if(o)*this=*o;else reset();return *this;}
    optional&operator=(optional&&o){if(o){if(valid_)*ptr()=move(*o);else{new(ptr())T(move(*o));valid_=true;}}else reset();return *this;}
    explicit operator bool()const{return valid_;}T&operator*(){return *ptr();}const T&operator*()const{return *ptr();}T*operator->(){return ptr();}const T*operator->()const{return ptr();}
};
template<class>class function;
template<class R,class...A>class function<R(A...)> {
    struct Base {virtual R invoke(A...)=0;virtual Base*clone()const=0;virtual~Base()=default;};
    template<class F>struct Held:Base {F f;Held(F value):f(move(value)){}R invoke(A...a)override{return f(forward<A>(a)...);}Base*clone()const override{return new Held(f);}};
    Base*p_=nullptr;ag::Cleanup cleanup_;static void destroy(void*p){static_cast<function*>(p)->~function();}void attach(){cleanup_.attach(this,destroy);}
public:
    function(){attach();}template<class F>function(F f):p_(new Held<F>(move(f))){attach();}function(const function&f):p_(f.p_?f.p_->clone():nullptr){attach();}function(function&&f)noexcept:p_(f.p_){f.p_=nullptr;attach();}
    ~function(){cleanup_.detach();delete p_;}R operator()(A...a)const{return p_->invoke(forward<A>(a)...);}
};

using ::floor;using ::sin;using ::cos;using ::trunc;using ::nextafter;
inline ag::Real strtod(const char*s,char**p){return ag::parse_real(s,p);}
inline ag::Real floor(ag::Real v){ag::Real r;r.raw=f64_roundToInt(v.raw,softfloat_round_min,false);return r;}
inline ag::Real trunc(ag::Real v){ag::Real r;r.raw=f64_roundToInt(v.raw,softfloat_round_minMag,false);return r;}
inline ag::Real abs(ag::Real v){return ag::Real::bits(v.raw.v&0x7fffffffffffffffull);}
inline bool isfinite(ag::Real v){return (v.raw.v&0x7ff0000000000000ull)!=0x7ff0000000000000ull;}
inline float floor(float v){return floorf(v);}inline float sin(float v){return sinf(v);}inline float cos(float v){return cosf(v);}inline float nextafter(float a,float b){
    // The installed SDK's nextafterf does not implement adjacent IEEE values.
    if(a!=a||b!=b)return a+b;if(a==b)return b;
    uint32_t bits;memcpy(&bits,&a,4);
    if(a==0){uint32_t sign;memcpy(&sign,&b,4);bits=(sign&0x80000000u)|1u;}
    else if((a>0)==(a<b))++bits;else --bits;
    memcpy(&a,&bits,4);return a;
}
inline float abs(float v){return fabsf(v);}inline double abs(double v){return fabs(v);}
template<class T,class=enable_if_t<is_integral<T>::value>>T abs(T v){return v<0?-v:v;}
template<class T>bool isfinite(T v){return __builtin_isfinite(v);}template<class T>int fpclassify(T v){return __builtin_fpclassify(FP_NAN,FP_INFINITE,FP_NORMAL,FP_SUBNORMAL,FP_ZERO,v);}
template<class T>struct numeric_limits;
template<>struct numeric_limits<float>{static constexpr bool is_iec559=true;static constexpr float max(){return FLT_MAX;}static constexpr float infinity(){return __builtin_inff();}};

using streamsize=ptrdiff_t;namespace ios {constexpr int binary=1;}
class ostream {
protected: string text_;FILE*file_=nullptr;bool good_=true;int precision_=6;
    void emit(const char*p,size_t n){if(file_){
#ifdef AGONDEV
        // SDK fwrite special-cases these pointers through putchar(), which
        // ignores freopen(stderr). fputc honours the selected FILE handle.
        if(file_==stdout||file_==stderr){for(size_t i=0;i<n;++i)if(fputc((unsigned char)p[i],file_)==EOF)good_=false;return;}
#endif
        if(fwrite(p,1,n,file_)!=n)good_=false;}else text_+=string(p,n);}
public:
    ostream()=default;explicit ostream(FILE*f):file_(f){}
    ostream&operator<<(const string&s){emit(s.data(),s.size());return *this;}ostream&operator<<(const char*s){emit(s,strlen(s));return *this;}ostream&operator<<(char ch){emit(&ch,1);return *this;}
    ostream&operator<<(unsigned char ch){char v=(char)ch;emit(&v,1);return *this;}
    template<class T,class=enable_if_t<is_integral<T>::value>>ostream&operator<<(T v){return *this<<to_string(v);}
    ostream&operator<<(ag::Real v){char b[100];unsigned n=ag::format_real(v,(unsigned)precision_,b,sizeof b);emit(b,n);return *this;}
    ostream&operator<<(double v){return *this<<ag::Real(v);}
    ostream&write(const char*p,streamsize n){emit(p,(size_t)n);return *this;}
    explicit operator bool()const{return good_;}bool operator!()const{return !good_;}
    void precision(int p){precision_=p;}
};
struct Precision{int n;};inline Precision setprecision(int n){return {n};}inline ostream&operator<<(ostream&o,Precision p){o.precision(p.n);return o;}
class ostringstream:public ostream {public:string str()const{return text_;}};
class ifstream {
    FILE*file_=nullptr;ag::Cleanup cleanup_;static void destroy(void*p){static_cast<ifstream*>(p)->~ifstream();}
public:
    ifstream(const string&s,int=0):file_(fopen(s.c_str(),"rb")){cleanup_.attach(this,destroy);}~ifstream(){cleanup_.detach();if(file_)fclose(file_);}explicit operator bool()const{return file_!=nullptr;}bool operator!()const{return !file_;}ifstream*rdbuf(){return this;}int get(){return file_?fgetc(file_):EOF;}
};
inline ostream&operator<<(ostream&o,ifstream*in){int ch;while((ch=in->get())!=EOF)o<<(char)ch;return o;}
class ofstream:public ostream {
    ag::Cleanup cleanup_;static void destroy(void*p){static_cast<ofstream*>(p)->~ofstream();}
public:ofstream(const string&s,int=0){file_=fopen(s.c_str(),"wb");good_=file_!=nullptr;cleanup_.attach(this,destroy);}~ofstream(){cleanup_.detach();close();}void close(){if(file_){if(fclose(file_))good_=false;file_=nullptr;}}
};
template<class T>class istreambuf_iterator {
    ifstream*in_=nullptr;int ch_=EOF;
public:istreambuf_iterator()=default;explicit istreambuf_iterator(ifstream&i):in_(&i){++*this;}T operator*()const{return (T)ch_;}istreambuf_iterator&operator++(){ch_=in_?in_->get():EOF;if(ch_==EOF)in_=nullptr;return *this;}bool operator!=(const istreambuf_iterator&i)const{return in_!=i.in_;}bool operator==(const istreambuf_iterator&i)const{return in_==i.in_;}
};
class istringstream {string text_;size_t at_=0;public:explicit istringstream(const string&s):text_(s){}bool line(string&s){if(at_==text_.size())return false;size_t end=text_.find('\n',at_);if(end==string::npos)end=text_.size();s=text_.substr(at_,end-at_);at_=end<text_.size()?end+1:end;return true;}};
inline bool getline(istringstream&i,string&s){return i.line(s);}
// AgonDev's bundled CRT does not walk C++ .init_array correctly. Construct
// streams on first use, after main has established its recovery context.
inline ostream& out_stream(){static ostream value(stdout);return value;}
inline ostream& err_stream(){static ostream value(stderr);return value;}
namespace filesystem {
class path {
    std::string value_;
public:path()=default;path(const char*s):value_(s){}path(const std::string&s):value_(s){}std::string string()const{return value_;}operator std::string()const{return value_;}
    path operator/(const path&p)const{return path(value_+(value_.empty()?"":"/")+p.value_);}
};
inline bool exists(const path&p){
#ifdef AGONDEV
    FILINFO info;return !ffs_stat(&info,p.string().c_str());
#else
    struct stat st;return !stat(p.string().c_str(),&st);
#endif
}
inline void create_directories(const path&p){string s=p.string();for(size_t i=1;i<=s.size();++i)if(i==s.size()||s[i]=='/'){string part=s.substr(0,i);if(!exists(path(part))){
#ifdef AGONDEV
    if(mos_mkdir(part.c_str()))ag::panic("cannot create output directory");
#else
    if(mkdir(part.c_str(),0777))ag::panic("cannot create output directory");
#endif
}}}
}
// Large backend metadata is output, not compiler working state. Spool it so
// a 74 KB JSON document does not consume a 128 KB growing string plus copies.
class spoolstream:public ostream {
    ag::Cleanup cleanup_;static void destroy(void*p){static_cast<spoolstream*>(p)->~spoolstream();}
public:
    spoolstream(){
#ifdef AGONDEV
        file_=fopen("/golem/cache/backend.tmp","w+b");
#else
        file_=tmpfile();
#endif
        if(!file_)ag::panic("cannot open compiler metadata spool");cleanup_.attach(this,destroy);
    }
    ~spoolstream(){cleanup_.detach();if(file_)fclose(file_);file_=nullptr;
#ifdef AGONDEV
        remove("/golem/cache/backend.tmp");
#endif
    }
    void save(const filesystem::path&path){
        if(!good_||fflush(file_))ag::panic("cannot flush compiler metadata spool");
        long remaining=ftell(file_);if(remaining<0||fseek(file_,0,SEEK_SET))ag::panic("cannot rewind compiler metadata spool");
        FILE*out=fopen(path.string().c_str(),"wb");if(!out)ag::panic("cannot publish backend metadata");
        char bytes[512];size_t n;bool good=true;
        while(remaining){size_t want=remaining>(long)sizeof bytes?sizeof bytes:(size_t)remaining;
            n=fread(bytes,1,want,file_);if(n!=want||fwrite(bytes,1,n,out)!=n){good=false;break;}remaining-=(long)n;}
        if(ferror(file_))good=false;int closeResult=fclose(out);if(closeResult)good=false;if(!good)ag::panic("cannot write backend metadata");
    }
};
}
