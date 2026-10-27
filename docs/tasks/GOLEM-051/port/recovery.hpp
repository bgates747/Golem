// Explicit error recovery for this compiler's known try/catch sites.
#pragma once
#include "mini.hpp"
namespace ag {
struct Error {
    void *value=nullptr;const void *tag=nullptr;std::exception *base=nullptr;
    void (*destroy)(void*)=nullptr;
    ~Error(){if(destroy)destroy(value);}
};
template<class T>const void*type_tag(){static char value;return &value;}
template<class T>bool matches(const Error&e){if constexpr(__is_base_of(std::exception,T)){
    if constexpr(__is_same(T,std::exception))return e.base!=nullptr;
    return e.tag==type_tag<T>();
}else return e.tag==type_tag<T>();}
struct Frame {
    jmp_buf env;Frame*parent;Cleanup*mark;Error*previous;Error*error=nullptr;
    bool handling=false;
    bool(*accept)(const Error&);
    explicit Frame(bool(*)(const Error&));~Frame();
};
extern Frame *frame;extern Error *error;extern uintptr_t stack_high;
bool emergency_error(const Error*);
template<class T>auto filter(){return &matches<T>;}
[[noreturn]]void deliver(Error*,Frame*);
template<class T>[[noreturn]]void raise(const T&v){
    Error*e=new Error();e->value=new T(v);e->tag=type_tag<T>();e->destroy=[](void*p){delete static_cast<T*>(p);};
    if constexpr(__is_base_of(std::exception,T))e->base=static_cast<T*>(e->value);
    deliver(e,frame);
}
template<class T>T&caught(){if constexpr(__is_base_of(std::exception,T))return static_cast<T&>(*error->base);else return *static_cast<T*>(error->value);}
[[noreturn]]void rethrow();
}
