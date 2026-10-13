// Grammar and reference checks beyond the emitted-program vector suite.
#include "portable.hpp"
#include <cassert>
#include <iostream>
int main() {
    const std::string prefix = "uint16 a=0 uint16 b=1 uint16 result=0 ";
    unsigned rejected = 0, accepted = 0;
    auto bad = [&](const std::string &source) {
        try { portable::Parser(source).parse(); }
        catch (const golem::syntax::Error &e) {
            assert(std::string(e.what()).find("line ") != std::string::npos);
            ++rejected; return;
        }
        throw std::runtime_error("unexpectedly accepted: " + source);
    };
    for (const char *condition : {"a = b", "a ! b", "a <> b", "a =< b", "a => b",
            "a ! = b", "a = = b", "a < = b", "a > = b", "a < b < 3", "a", "a < 1.5"})
        bad(prefix + "Sub test If " + condition + " Then result=1 EndIf EndSub");
    for (const char *body : {"If a < b result=1 EndIf", "If a < b Then result=1",
            "Else result=1", "ElseIf a < b Then result=1", "EndIf", "If a < b Then Else Else EndIf",
            "If a < b Then Else ElseIf a < b Then EndIf", "If a < b Then result=1; EndIf",
            "If 65535 + 1 < b Then result=1 EndIf",
            "If a < b Then bool flag = 1 EndIf", "If a < b Then Do result=1 Loop EndIf"}) {
        bad(prefix + "Sub test " + body + " EndSub");
    }
    bad("float32 a=0.0 uint16 b=1 Sub test If a < b Then b=0 EndIf EndSub");
    for (unsigned depth : {8u,9u}) {
        std::string body;
        for (unsigned i=0;i<depth;++i) body+="If a < b Then ";
        body+="result=1 ";
        for (unsigned i=0;i<depth;++i) body+="EndIf ";
        const auto source=prefix+"Sub test "+body+"EndSub";
        if (depth==9) bad(source);
        else {
            auto program=portable::Parser(source).parse();auto layout=portable::layout(program);
            auto state=layout.initial;assert(portable::invokeReference(program,layout,state,0)==0);
            assert(portable::get(state,layout.offsets[2],portable::Type::U16).bits==1);++accepted;
        }
    }
    std::string chain="Sub test If a == 0 Then result=1 ";
    for(unsigned i=0;i<65;++i)chain+="ElseIf a == 1 Then result=2 ";
    bad(prefix+chain+"EndIf EndSub");
    for (const char *op : {"==","!=","<",">","<=",">="}) {
        portable::Parser(prefix+"sUb test iF (a + 1) "+op+" (b * 1) tHeN result=1 eLsE result=2 eNdIf eNdSuB").parse();
        ++accepted;
    }
    portable::Parser(prefix+"Sub test If a < b Then EndIf result=1 EndSub").parse();++accepted;
    for (const char *body : {
        "For missing In Range(0:3) Next missing",
        "For a Range(0:3) Next a", "For a In Range(0,3) Next a",
        "For a In Range(b:3) Next a", "For a In Range(0:3:b) Next a",
        "For a In Range(-1:3) Next a", "For a In Range(0:65536) Next a",
        "For a In Range(0:3:0) Next a", "For a In Range(0:0:0) Next a",
        "For a In Range(0:3:-1) Next a", "For a In Range(3:0) Next a",
        "For a In Range(0:3:65536) Next a", "For a In Range(0:3:-65536) Next a",
        "For a In Range(0:3:0.5) Next a", "For a In Range(0:3) Next b",
        "For a In Range(0:3) Next A",
        "For a In Range(0:3) a=0 Next a",
        "For a In Range(0:3) If a==0 Then a=0 EndIf Next a",
        "For a In Range(0:3) For a In Range(0:1) Next a Next a",
        "For a In Range(0:3) For b In Range(0:1) a=0 Next b Next a",
        "For a In Range(0:65535) For b In Range(0:65535) Next b Next a"})
        bad(prefix+"Sub test "+body+" EndSub");
    bad("float32 a=0.0 Sub test For a In Range(0:3) Next a EndSub");
    bad("uint16 a Bounds(2,4)=2 Sub test For a In Range(1:4) Next a EndSub");
    bad("uint16 a Bounds(2,4)=2 Sub test For a In Range(2:5) Next a EndSub");
    std::string declarations,body;
    for(unsigned i=0;i<9;++i)declarations+="uint16 c"+std::to_string(i)+"=0 ";
    for(unsigned i=0;i<9;++i)body+="For c"+std::to_string(i)+" In Range(0:0) ";
    for(int i=8;i>=0;--i)body+="Next c"+std::to_string(i)+" ";
    bad(declarations+"Sub test "+body+"EndSub");
    for (const char *range : {"0:9:2","9:0:-2","65535:65535","65535:65535:-2",
                             "0:65535:65535","65535:0:-65535","0:65535","65535:0:-1"}) {
        auto program=portable::Parser(prefix+"Sub test For a In Range("+range+") Next a EndSub").parse();
        auto layout=portable::layout(program);auto state=layout.initial;
        assert(portable::invokeReference(program,layout,state,0)==0);
        const auto &loop=program.procedures[0].statements[0];
        assert(portable::get(state,layout.offsets[0],portable::Type::U16).bits==unsigned(loop.last));
        ++accepted;
    }
    portable::Parser(prefix+"Sub test For a In Range(0:3) Next a a=0 For a In Range(2:0:-1) Next a EndSub").parse();++accepted;
    for (const char *body : {"For a In Range(0:3) Next", "For a In Range(0:3) Next result=9",
                            "For a In Range(0:3) Next a=9"}) {
        auto program=portable::Parser(prefix+"Sub test "+body+" EndSub").parse();
        auto layout=portable::layout(program);auto state=layout.initial;
        assert(portable::invokeReference(program,layout,state,0)==0);
        ++accepted;
    }
    portable::Parser("uint16 a Bounds(10,14)=10 Sub test For a In Range(10:15:2) Next a EndSub").parse();++accepted;
    body.clear();
    for(unsigned i=0;i<8;++i)body+="For c"+std::to_string(i)+" In Range(0:0) ";
    for(int i=7;i>=0;--i)body+="Next c"+std::to_string(i)+" ";
    portable::Parser(declarations+"Sub test "+body+"EndSub").parse();++accepted;
    std::cout << accepted << " accepted grammar/reference cases; " << rejected << " rejections\n";
}
