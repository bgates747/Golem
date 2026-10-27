#include "recovery.hpp"
void*operator new(std::size_t n){void*p=ag::memory(n?n:1);if(!p)ag::panic("out of compiler memory");return p;}
void*operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{free(p);}void operator delete[](void*p)noexcept{free(p);}
void operator delete(void*p,std::size_t)noexcept{free(p);}void operator delete[](void*p,std::size_t)noexcept{free(p);}
namespace ag {
const char*phase="reading source";
void*memory(size_t n){void*p=malloc(n);if(!p)fprintf(stderr,"Allocation request: %lu bytes\n",(unsigned long)n);return p;}
Frame*frame=nullptr;Error*error=nullptr;uintptr_t stack_high=0;
static Cleanup*head=nullptr;
void Cleanup::attach(void*p,void(*d)(void*)){
    char local;uintptr_t address=(uintptr_t)p;
    if(address<=(uintptr_t)&local||address>stack_high)return;
    object=p;destroy=d;next=head;head=this;
}
void Cleanup::detach(){if(!object)return;Cleanup**link=&head;while(*link&&*link!=this)link=&(*link)->next;if(*link)*link=next;object=nullptr;}
Frame::Frame(bool(*a)(const Error&)):parent(frame),mark(head),previous(ag::error),accept(a){frame=this;}
Frame::~Frame(){frame=parent;if(error&&error!=previous&&!emergency_error(error))delete error;ag::error=previous;}
[[noreturn]]void deliver(Error*e,Frame*f){
    while(f&&(f->handling||!f->accept(*e))){
        Frame*discard=f;f=f->parent;
        if(discard->error&&discard->error!=discard->previous&&discard->error!=e&&!emergency_error(discard->error))delete discard->error;
    }
    if(!f){fputs(e->base?e->base->what():"unhandled compiler error",stderr);fputc('\n',stderr);exit(1);}
    frame=f;
    while(head!=f->mark){Cleanup*p=head;if(!p)abort();void*object=p->object;auto d=p->destroy;d(object);}
    f->error=e;f->handling=true;error=e;longjmp(f->env,1);
}
[[noreturn]]void rethrow(){Error*e=error;Frame*parent=frame?frame->parent:nullptr;if(frame)frame->error=nullptr;deliver(e,parent);}
static Error*emergency=nullptr;
bool emergency_error(const Error*e){return e==emergency;}
[[noreturn]]void panic(const char*s){
    if(phase)fprintf(stderr,"Compiler stage: %s\n",phase);
    struct Emergency:std::exception{const char*message=nullptr;const char*what()const noexcept override{return message;}};
    static Emergency value;static Error record;value.message=s;record.value=&value;record.base=&value;record.tag=type_tag<Emergency>();emergency=&record;
    deliver(&record,frame);
}
}
