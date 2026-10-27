// Integer bit patterns, precedence, admission and diagnostic contracts.
#include "../src/portable/portable.hpp"
#include <cassert>
#include <iostream>
using namespace portable;
unsigned checks=0;
void run(const std::string &source,const std::vector<std::pair<std::string,uint32_t>>&expected,unsigned status=0) {
    const auto p=Parser(source).parse();const auto l=layout(p);auto state=l.initial;
    ArrayState arrays;for(size_t i=0;i<p.arrays.size();++i)arrays[l.arrayBuffers[i]]=p.arrays[i].initial;
    assert(invokeReference(p,l,state,0,&arrays)==status);
    for(const auto &v:expected) {
        size_t i=0;while(i<p.globals.size()&&p.globals[i].name!=v.first)++i;
        assert(i<p.globals.size());assert(get(state,l.offsets[i],p.globals[i].type).bits==v.second);++checks;
    }
}
int main() {
    for(auto type:{Type::U16,Type::S16,Type::U32,Type::S32}) {
        const uint32_t mask=width(type)==2?65535u:UINT32_MAX;
        auto spelling=[&](uint32_t bits){return std::to_string(static_cast<int64_t>(Value{type,bits&mask}.number()));};
        for(auto pair:std::vector<std::pair<uint32_t,uint32_t>>{{0,0},{0,UINT32_MAX},{UINT32_MAX,UINT32_MAX},
                {0x55555555u,0xaaaaaaaau},{0x80008000u,1},{0x12345678u,0x87654321u}}) {
            const auto a=pair.first&mask,b=pair.second&mask;
            const std::string t=name(type),prefix=t+" a="+spelling(a)+" "+t+" b="+spelling(b)+" "+t+" x=0 "+t+" y=0 "+t+" z=0 "+t+" n=0 ";
            run(prefix+"Sub f x=a&b y=a|b z=a^b n=~a EndSub",{{"a",a},{"b",b},{"x",a&b},{"y",a|b},{"z",a^b},{"n",~a&mask}});
            const auto folded=Parser(prefix+"Sub f x="+spelling(a)+"&"+spelling(b)+" y="+spelling(a)+"|"+spelling(b)+" n=~"+spelling(a)+" EndSub").parse();
            assert(folded.procedures[0].statements[0].value->constant->bits==(a&b));
            assert(folded.procedures[0].statements[1].value->constant->bits==(a|b));
            assert(folded.procedures[0].statements[2].value->constant->bits==((~a)&mask));checks+=3;
        }
    }
    run("uint16 a=1 uint16 b=2 uint16 c=3 uint16 x=0 uint16 y=0 bool flag=false "
        "Sub f x=a|b^c&a y=~a*0 flag=Not a|b==0 And a&b==0 Or false EndSub",
        {{"x",3},{"y",0},{"flag",1}});
    run("uint32 a=64 uint16 bits=2 uint32 x=0 uint32 y=0 "
        "Sub f x=a>>bits&15|3^1 y=1+2*3|8 EndSub",{{"x",2},{"y",15}});
    run("uint16 a=1 uint16 b=2 uint16 c=3 uint16 x=0 Sub f x=(a|b)^c EndSub",{{"x",0}});
    run("sint16 a=-32768 sint16 x=0 Sub f x=~a EndSub",{{"x",32767}});
    run("sint32 a=-2147483648 sint32 x=0 Sub f x=~a EndSub",{{"x",2147483647}});
    run("uint16 a=1 uint16 x=0 Sub f x=~~~~~~~~~~~~~~~~a EndSub",{{"x",1}});
    run("uint16 a=65535 uint16 x Bounds(0,15)=7 Sub f x=a|0 EndSub",{{"x",7}},32);
    run("uint16 a=0 uint16 index=1 uint16 x=7 Array uint16 data(0:0)=Fill(1) Sub f x=a&data(index) EndSub",{{"x",7}},5);
    run("uint16 a=65535 uint16 x=7 Sub f x=a|(a+1) EndSub",{{"x",7}},1);
    run("uint16 a=1 uint16 x=7 Sub f x=~(a+65535) EndSub",{{"x",7}},1);
    run("uint16 a=0 uint16 index=1 bool flag=false Array uint16 data(0:0)=Fill(1) "
        "Sub f flag=a==0 Or a&data(index)==0 EndSub",{{"flag",1}});
    const auto p=Parser("uint16 a=1 uint16 b=2 uint16 c=3 Sub f a=a&b&c a=a|b|c EndSub").parse();
    assert(p.procedures[0].statements[0].value->left->op==Op::BitAnd);
    assert(p.procedures[0].statements[1].value->left->op==Op::BitOr);
    unsigned rejected=0;
    for(auto text:{"float32 a=1.0 Sub f a=a&1.0 EndSub","float32 a=1.0 Sub f a=a|1.0 EndSub",
            "float32 a=1.0 Sub f a=~a EndSub","bool a=true Sub f a=a&false EndSub",
            "bool a=true Sub f a=a|false EndSub","bool a=true Sub f a=~a EndSub",
            "uint16 a=1 uint32 b=1 Sub f a=a&b EndSub","uint16 a=1 sint16 b=1 Sub f a=a|b EndSub",
            "uint32 a=1 sint32 b=1 Sub f a=a&b EndSub","uint16 a=1 Sub f a=a&&1 EndSub",
            "uint16 a=1 Sub f a=a||1 EndSub","uint16 a=1 Sub f a=a| EndSub",
            "uint16 a=1 Sub f a=~ EndSub","uint16 a=1 Sub f a=a&65536 EndSub",
            "uint16 a=~1 Sub f a=0 EndSub","uint16 a=1 Sub f a=~~~~~~~~~~~~~~~~~a EndSub",
            "uint16 a=1 Sub f a=a|true EndSub","uint16 a=1 Sub f a=a&=1 EndSub",
            "uint16 a=1 Sub f a=~1.5 EndSub","uint16 a=1 Sub f a=a<<1 EndSub"}) {
        bool failed=false;try{Parser(text).parse();}catch(const std::exception&){failed=true;}
        assert(failed);++rejected;
    }
    std::cout<<"Bitwise contracts pass: "<<checks<<" values/folds, "<<rejected<<" rejections.\n";
}
