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
    std::cout << accepted << " accepted grammar/reference cases; " << rejected << " rejections\n";
}
