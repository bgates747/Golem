#include "portable.hpp"
#include <fstream>
#include <iostream>
#include <iterator>

namespace {
unsigned checks = 0, rejections = 0;
void check(bool condition) {
    if (!condition) throw std::runtime_error("host contract check failed at check " + std::to_string(checks + 1));
    ++checks;
}
template<class Action> void reject(Action action, const std::string &text) {
    try { action(); }
    catch (const std::exception &error) {
        if (std::string(error.what()).find(text) == std::string::npos)
            throw std::runtime_error("expected " + text + "; got " + error.what());
        ++rejections;
        return;
    }
    throw std::runtime_error("unexpected acceptance: " + text);
}
}
int main(int argc, char **argv) {
    using namespace portable;
    try {
        if (argc != 2) throw std::runtime_error("supply shared-example.golem");
        std::ifstream file(argv[1]);
        if (!file) throw std::runtime_error("could not read example");
        const std::string source{std::istreambuf_iterator<char>(file), {}};
        const auto program = Parser(source).parse();
        const auto planned = layout(program);
        check(program.globals.size() == 7 && program.procedures.size() == 2);
        check(planned.buffer == 1 && planned.bytes == 24 && planned.errorOffset == 22);
        check(planned.offsets == std::vector<unsigned>({0, 2, 4, 6, 10, 14, 18}));
        check(get(planned.initial, 6, Type::F32).bits == 0x40000000);
        check(get(planned.initial, 10, Type::F32).bits == 0x3fc00000);
        check(get(planned.initial, 14, Type::F32).bits == 0x3e800000);
        const auto relocated = layout(program, {{1, 99}, {150, 159}});
        check(relocated.buffer == 100 && relocated.initial == planned.initial);
        check(relocated.offsets == planned.offsets && relocated.errorOffset == planned.errorOffset);
        auto state = planned.initial;
        check(invokeReference(program, planned, state, 0) == Ok);
        check(get(state, 0, Type::U16).bits == 1 && get(state, 4, Type::U16).bits == 6);
        check(invokeReference(program, planned, state, 0) == Ok);
        check(get(state, 0, Type::U16).bits == 2 && get(state, 4, Type::U16).bits == 12);
        check(invokeReference(program, planned, state, 1) == Ok);
        check(get(state, 18, Type::F32).number() == 3.25);
        put(state, 6, floating(-4.0f));
        check(invokeReference(program, planned, state, 1) == Ok);
        check(get(state, 18, Type::F32).number() == -5.75);
        // Failed first assignment preserves both application destinations.
        put(state, 0, {Type::U16, 65535});
        check(invokeReference(program, planned, state, 0) == AddOverflow);
        check(get(state, 0, Type::U16).bits == 65535 && get(state, 4, Type::U16).bits == 12);
        check(get(state, planned.errorOffset, Type::U16).bits == AddOverflow);
        put(state, 0, {Type::U16, 4});
        check(invokeReference(program, planned, state, 0) == Ok);
        check(get(state, planned.errorOffset, Type::U16).bits == Ok);
        // Earlier committed calls update survives failure of accumulated update.
        put(state, 4, {Type::U16, 65535});
        check(invokeReference(program, planned, state, 0) == AddOverflow);
        check(get(state, 0, Type::U16).bits == 6 && get(state, 4, Type::U16).bits == 65535);
        state = planned.initial;
        put(state, 2, {Type::U16, 101});
        check(invokeReference(program, planned, state, 0) == BoundsFailure);
        check(get(state, 0, Type::U16).bits == 0 && get(state, 2, Type::U16).bits == 101);
        state = planned.initial;
        put(state, 6, {Type::F32, 0x7fc00001});
        check(invokeReference(program, planned, state, 1) == NonFinite);
        check(get(state, 18, Type::F32).bits == 0 && get(state, 6, Type::F32).bits == 0x7fc00001);

        auto run = [&](const std::string &text, unsigned expected, unsigned offset, unsigned result) {
            const auto p = Parser(text).parse();
            const auto l = layout(p);
            auto s = l.initial;
            check(invokeReference(p, l, s, 0) == expected);
            check(get(s, offset, Type::U16).bits == result);
            return s;
        };
        run("uint16 a=256 uint16 result=99 Sub go result=a*a EndSub", MultiplyOverflow, 2, 99);
        const auto partial = run("uint16 a=0 uint16 b=7 uint16 c=9 Sub go b=b+1 c=a-1 a=1 EndSub", SubUnderflow, 4, 9);
        check(get(partial, 0, Type::U16).bits == 0 && get(partial, 2, Type::U16).bits == 8);
        run("uint16 a=5 uint16 out Bounds(0,9)=7 Sub go out=a*2 EndSub", BoundsFailure, 2, 7);
        run("uint16 a=2 uint16 out=0 Sub go out=1+a*3-2 EndSub", Ok, 2, 5);
        run("uint16 a=2 uint16 out=0 Sub go out=(1+a)*3-2 EndSub", Ok, 2, 7);
        const auto huge = Parser("float32 x=3e38 float32 out=1.0 Sub go out=x*2.0 EndSub").parse();
        const auto hugeLayout = layout(huge);
        auto hugeState = hugeLayout.initial;
        check(invokeReference(huge, hugeLayout, hugeState, 0) == NonFinite);
        check(get(hugeState, 4, Type::F32).number() == 1.0);
        const auto signedZero = Parser("float32 x=-0.0 Sub go x=x EndSub").parse();
        check(layout(signedZero).initial.at(3) == 0x80);
        const auto upper = Parser("uint16 x=65535 Sub go x=x EndSub").parse();
        check(get(layout(upper).initial, 0, Type::U16).bits == 65535);
        const auto formatted = Parser("UINT16 MiXeD=1 sUb tick MiXeD=MiXeD+1 eNdSuB").parse();
        check(formatted.globals[0].name == "MiXeD" && formatted.procedures[0].name == "tick");
        check(formatted.procedures[0].statements[0].at.line == 1);

        const std::vector<std::pair<std::string, std::string>> bad{
            {"uint16 x Sub go x=1 EndSub", "expected '='"},
            {"uint16 x=0; Sub go x=1 EndSub", "remove ';'"},
            {"uint16 x=0 uint16 x=1 Sub go x=1 EndSub", "duplicate declaration"},
            {"uint16 x=0 Sub x x=1 EndSub", "duplicate declaration"},
            {"uint16 x=0 Sub go x=X EndSub", "undeclared global"},
            {"uint16 x=0 Sub go missing=1 EndSub", "undeclared global"},
            {"uint16 x=0 float32 y=1 Sub go x=y EndSub", "mixed variable types"},
            {"float32 x=0 uint16 y=1 Sub go x=y EndSub", "mixed variable types"},
            {"uint16 x=65536 Sub go x=1 EndSub", "literal does not fit"},
            {"uint16 x=-1 Sub go x=1 EndSub", "literal does not fit"},
            {"uint16 x=0.5 Sub go x=1 EndSub", "literal does not fit"},
            {"uint16 x Bounds(10,0)=1 Sub go x=1 EndSub", "invalid inclusive Bounds"},
            {"uint16 x Bounds(0,0.5)=0 Sub go x=1 EndSub", "invalid inclusive Bounds"},
            {"uint16 x Bounds(0,9)=10 Sub go x=1 EndSub", "initializer is outside"},
            {"float32 x Bounds(0,0.2)=0.2 Sub go x=0 EndSub", "after rounding"},
            {"float32 x=1e-50 Sub go x=0 EndSub", "float32 range"},
            {"float32 x=1e40 Sub go x=0 EndSub", "float32 range"},
            {"uint16 x=0 Sub go x=65535+1-1 EndSub", "status 1"},
            {"uint16 x=0 Sub go x=0-1+1 EndSub", "status 2"},
            {"uint16 x=0 Sub go x=256*256*0 EndSub", "status 3"},
            {"float32 x=0 Sub go x=3e38*2 EndSub", "status 33"},
            {"uint16 x Bounds(0,9)=0 Sub go x=10 EndSub", "constant assignment"},
            {"uint16 runtime_error=0 Sub go runtime_error=1 EndSub", "reserved declaration"},
            {"uint16 Bounds=0 Sub go Bounds=1 EndSub", "reserved declaration"},
            {"uint16 x=0 Sub go EndSub", "empty Sub"},
            {"uint16 x=0 Sub go x=1", "expected 'EndSub'"},
            {"uint16 x=0 Sub go x=1 EndSub uint16 y=0", "after Subs"},
            {"uint16 x=0 Sub go x=x/2 EndSub", "only scalar global assignments"},
            {"uint16 x=0 Sub go If x==0 Then x=1 EndIf EndSub", "only scalar global assignments"},
            {"uint16 x=0 Sub go uint16 local=0 EndSub", "only scalar global assignments"},
            {"uint16 x=0 Sub go x=(x+1 EndSub", "expected ')'"},
            {"uint16 x=0 Sub go x=1 EndSub Sub go x=2 EndSub", "duplicate declaration"},
        };
        for (const auto &item : bad) reject([&] { Parser(item.first).parse(); }, item.second);
        reject([&] { layout(program, {{0, 1}}); }, "reserved");
        reject([&] { layout(program, {{1, 65535}}); }, "reserved");
        reject([&] { layout(program, {{1, 3}, {3, 4}}); }, "already claimed");
        reject([&] { layout(program, {{10, 1}}); }, "reversed");
        reject([&] { layout(program, {{1, 65534}}); }, "exhausted");
        const auto collision = Parser("uint16 X=0 uint16 x=0 Sub go x=X EndSub").parse();
        reject([&] { assembly(collision, layout(collision)); }, "symbol collision");
        const auto longName = std::string(65, 'a');
        const auto longProgram = Parser("uint16 " + longName + "=0 Sub go " + longName + "=1 EndSub").parse();
        reject([&] { assembly(longProgram, layout(longProgram)); }, "64-character");
        auto oversized = program;
        oversized.globals.resize(32767, program.globals.front());
        reject([&] { layout(oversized); }, "one stock WRITE block");
        reject([&] { auto wrong = state; wrong.pop_back(); invokeReference(program, planned, wrong, 0); }, "invalid host reference");
        reject([&] { invokeReference(program, planned, state, 99); }, "invalid host reference");
        std::cout << "Shared frontend/layout/HOST reference: " << checks << " checks, "
                  << rejections << " rejection cases pass. No target execution.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
