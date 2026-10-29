#include "../src/portable/driver.hpp"
#include <cassert>
#include <iostream>
#include <cstdio>
using namespace portable;
void check(const std::string &source,const std::string &expected,unsigned status=0) {
    auto p=Parser(source).parse();auto l=layout(p);auto state=l.initial;std::string output;
    ArrayState arrays;for(size_t i=0;i<p.arrays.size();++i)arrays[l.arrayBuffers[i]]=p.arrays[i].initial;
    assert(containsPrint(p));assert(invokeReference(p,l,state,0,&arrays,&output)==status);assert(output==expected);
}
int main(int argc,char**argv){
    assert(argc==2);namespace fs=std::filesystem;const fs::path work=argv[1];fs::create_directories(work);
    const auto source=work/"print.golem";
    {std::ofstream out(source);out<<"uint16 x=7 Sub f print str(x) EndSub";}
    auto compile=[&](const std::vector<std::string>&options,const fs::path&output){
        std::vector<std::string> args{"golemc","--portable"};args.insert(args.end(),options.begin(),options.end());
        args.push_back(source.string());args.push_back(output.string());std::vector<char*> ptrs;for(auto&a:args)ptrs.push_back(a.data());
        std::ostringstream sink;auto*out=std::cout.rdbuf(sink.rdbuf());auto*err=std::cerr.rdbuf(sink.rdbuf());
        const int result=golem::portable_cli::run(ptrs.size(),ptrs.data());std::cout.rdbuf(out);std::cerr.rdbuf(err);return result;
    };
    assert(compile({},work/"default")==0);
    assert(fs::exists(work/"default/buffered-install.bin")&&!fs::exists(work/"default/native.c"));
    assert(fs::exists(work/"default/targets.json"));
    assert(compile({"--target","buffered","--emit-load-plan"},work/"explicit")==0);
    assert(fs::exists(work/"explicit/load-plan.json"));
    for(auto target:{"native","both"}){const auto out=work/target;assert(compile({"--target",target},out)==1);assert(!fs::exists(out));}

    check("Sub hello print \"hello\" print \"\" print \" world\" EndSub","hello world");
    check("uint16 a=65535 sint16 b=-32768 uint32 c=4294967295 sint32 d=-2147483648 "
          "Sub f print str(a) print \" \" print str(b) print \" \" print str(c) print \" \" print str(d) EndSub",
          "65535 -32768 4294967295 -2147483648");
    check("uint16 i=0 uint16 total=0 Sub f For i In Range(1:10) total=total+i Next i "
          "print \"i=\" print str(i) print \" total=\" print str(total) EndSub","i=10 total=55");
    check("uint16 x=0 Sub f print str(0) print \"/\" print str(-1) print \"/\" print str(65536) "
          "print \"/\" print str(-2147483648) EndSub","0/-1/65536/-2147483648");
    check("uint16 i=0 Sub f For i In Range(0:2) print str(i) Next i If i==2 Then "
          "print \"yes\" Else print \"no\" EndIf EndSub","012yes");
    check("uint16 i=1 Array sint32 data(0:1)=Values(0,-2147483648) Sub f print str(data(i)) EndSub","-2147483648");
    check("uint16 x=65535 Sub f print \"before\" print str(x+1) print \"after\" EndSub","before",1);
    check("uint16 x=65535 Sub f print str(x) print str(x) EndSub","6553565535");
    check("float32 x=1.5 Sub f print str(x) EndSub","1.5");
    check("float32 x=0.1 Sub f print str(x) EndSub","0.100000001");
    check("float32 x=-0.0 Sub f print str(x) EndSub","0");
    check("float32 x=1000000000.0 Sub f print str(x) EndSub","1e+09");
    check("float32 x=0.00001 Sub f print str(x) EndSub","9.99999975e-06");
    check("float32 x=1.5 float32 y=2.0 Sub f print str(x*y+x) EndSub","4.5");
    check("Sub f print str(1.5) print \"/\" print str(-0.0) EndSub","1.5/0");
    uint32_t seed=5401;
    for(unsigned i=0;i<4096;++i) {
        seed=seed*1664525u+1013904223u;
        const auto magnitude=seed&0x7fffffffu;
        if(magnitude<0x00800000u || magnitude>=0x7f800000u)continue;
        const Value value{Type::F32,seed};char expected[32];
        std::snprintf(expected,sizeof expected,"%.9g",value.number());
        assert(decimal(value)==expected);
    }
    unsigned rejected=0;
    for(auto text:{"uint16 x=1 Sub f print x EndSub",
        "bool x=true Sub f print str(x) EndSub","uint16 x=1 Sub f print str(x,1) EndSub",
        "uint16 x=1 Sub f print str() EndSub","uint16 x=1 Sub f print str(\"1\") EndSub",
        "uint16 x=1 Sub f print str(x),str(x) EndSub","uint16 x=1 Sub f print \"a\"+\"b\" EndSub",
        "uint16 x=1 Sub f print str(x) & \"b\" EndSub","uint16 x=1 Sub f x=str(x) EndSub",
        "uint16 x=1 sint16 y=1 Sub f print str(x+y) EndSub","Sub f print str(unknown) EndSub"}) {
        bool bad=false;try{Parser(text).parse();}catch(const std::exception&){bad=true;}assert(bad);++rejected;
    }
    auto p=Parser("uint16 x=1 Sub f print str(x) EndSub").parse();auto l=layout(p);
    bool bad=false;try{dual::nativeC(p,l);}catch(const std::exception&){bad=true;}assert(bad);
    assert(programJson(p).find("\"print\":{\"integer\"")!=std::string::npos);
    auto floats=Parser("float32 x=1.5 Sub f print str(x) EndSub").parse();
    assert(containsFloatPrint(floats));assert(!containsFloatPrint(p));
    assert(programJson(floats).find("\"schema\":6")!=std::string::npos);
    assert(programJson(floats).find("\"print\":{\"float32\"")!=std::string::npos);
    {std::ofstream out(source);out<<"float32 x=1.5 Sub f print str(x) EndSub";}
    assert(compile({},work/"float-buffered")==0);
    assert(!fs::exists(work/"float-buffered/native.c"));
    for(auto target:{"native","both"}) {
        const auto out=work/(std::string("float-")+target);
        assert(compile({"--target",target},out)==1);assert(!fs::exists(out));
    }
    std::cout<<"Printing contracts pass: 15 output/error cases, 4096 float samples and "<<rejected<<" rejected forms.\n";
}
