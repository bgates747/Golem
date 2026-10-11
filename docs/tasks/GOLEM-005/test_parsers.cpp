#define main maintained_cli_main
#include "../../../src/golemc.cpp"
#undef main
// Exact pre-tranche compiler source is materialized by run_checks.py.
namespace baseline {
#define main baseline_cli_main
#include "build/legacy.cpp"
#undef main
}
#include "../GOLEM-003/scalar_frontend.hpp"
#include "scalar_frontend.hpp"
#include <cassert>
#include <limits>

namespace {
size_t ordinaryPairs = 0, scalarPairs = 0, rejections = 0;
void pair(const std::string &oldSource, const std::string &newSource) {
    assert(baseline::compile(oldSource) == compile(newSource));
    ++ordinaryPairs;
}
template<class F> void reject(F action, const std::string &message) {
    try { action(); }
    catch (const std::exception &error) {
        if (std::string(error.what()).find(message) == std::string::npos) {
            std::cerr << "Expected error containing: " << message << "\nActual error: " << error.what() << '\n';
            assert(false);
        }
        ++rejections; return;
    }
    assert(false && "source unexpectedly accepted");
}
std::string scalarSource(double value, const std::string &separator,
                         double lo = -32768, double hi = 32767) {
    std::ostringstream out;
    out << std::setprecision(17) << "float32 x Range(" << lo << ',' << hi << ")=" << value
        << separator << "sint16 result" << separator << "Sub compute" << separator
        << "result=sint16(floor(x))" << separator << "EndSub" << separator;
    return out.str();
}
void scalarPair(double value, double lo = -32768, double hi = 32767) {
    auto old = readable::compile(scalarSource(value, "; ", lo, hi));
    auto next = termless::compile(scalarSource(value, "\n", lo, hi));
    assert(old.bootstrap == next.bootstrap && old.cleanup == next.cleanup);
    assert(old.invoke == next.invoke && old.programs == next.programs);
    assert(scalar::inputUpdate(old, value) == scalar::inputUpdate(next, value));
    ++scalarPairs;
}
}
int main() {
    pair("", "// empty\n");
    pair("print \"foo\"; print \"bar\";", "print \"foo\" print \"bar\"");
    pair("print \"; // \\\" \\\\ \\n \\q\";", "print \"; // \\\" \\\\ \\n \\q\"");
    pair("uint16 a=1; uint16 b; b=a+2;", "uint16 a=1 uint16 b b=a+\n// continuation\n2");
    pair("uint16 a; a=1; a=a+2;", "uint16 a a=1 a=a+2");
    pair("UINT16 A=1; uint16 a=2; A=A+a;", "UINT16 A=1 uint16 a=2 A=A+a");
    pair("uint16 printout; printout=5;", "uint16 printout printout=5");
    pair("uint16 i; For i=1 To 2; print \"tick\"; Next i;",
         "uint16 i For i=1 To 2 print \"tick\" Next i");
    pair("uint16 i; uint16 j; uint16 a=2; For i=1 To a+2; For j=i To 3; a=a+1; Next j; Next i; print \"done\";",
         "uint16 i uint16 j uint16 a=2 For i=1 To a+2 For j=i To 3 a=a+1 Next j Next i print \"done\"");
    pair("uint16 i; fOr i=4 tO 2; NeXt i;", "uint16 i fOr i=4 tO 2 NeXt i");
    // Compile-only parity at the legacy endpoint; not firmware qualification.
    pair("uint16 i; For i=65535 To 65535; Next i;", "uint16 i For i=65535 To 65535 Next i");
    for (unsigned value : {0u, 1u, 255u, 256u, 32767u, 32768u, 65534u, 65535u}) {
        const auto literal = std::to_string(value);
        pair("uint16 a=" + literal + "; uint16 b; b=a+" + literal + "+1;",
             "uint16 a=" + literal + " uint16 b b=a+" + literal + "+1");
    }
    pair("uint16 a=00000000000000000000000000001;", "uint16 a=00000000000000000000000000001");
    for (const auto &text : {"uint16 a=65536", "uint16 a=9999999999999999999999999999999999"})
        reject([&] { compile(text); }, "does not fit");
    for (const auto &literal : {"1foo", "0x12", "1.2.3", "1e", "1e+", "1e-", "12_3", "1e2foo"})
        reject([&] { compile(std::string("uint16 a=") + literal); }, "malformed numeric literal");
    for (const auto &literal : {"1.5", "1e2", ".5"})
        reject([&] { compile(std::string("uint16 a=") + literal); }, "unsigned decimal integer");
    for (const auto &text : {"print \"foo\";", "uint16 a;", ";", "uint16 i For i=1 To 2; Next i"})
        reject([&] { compile(text); }, "remove ';'");
    for (const auto &text : {"uint16 a a=1+print \"foo\"", "uint16 a a=1+", "uint16 a a=+1", "uint16 a a=-1"})
        reject([&] { compile(text); }, "expression operand");
    reject([] { compile("uint16 a=1 b=2"); }, "undeclared variable 'b'");
    reject([] { compile("uint16 A=1 A=a"); }, "undeclared variable 'a'");
    reject([] { compile("uint16 a uint16 a"); }, "redeclaration");
    reject([] { compile("For i=1 To 2 Next i"); }, "undeclared variable");
    reject([] { compile("uint16 i For i=1 To 2 Next j"); }, "does not match");
    reject([] { compile("uint16 i For i=1 To 2"); }, "expected 'Next'");
    reject([] { compile("uint16 i For i=1 2 Next i"); }, "expected 'To'");
    reject([] { compile("Next i"); }, "no matching 'For'");
    for (const auto &word : {"If", "rAnGe", "Floor", "ElseIf", "Switch", "Sub", "Until"})
        reject([&] { compile(std::string("uint16 ") + word); }, "reserved identifier");
    for (const auto &word : {"If", "Else", "Case", "EndSub", "Do", "Switch"})
        reject([&] { compile(word); }, "unsupported statement or unexpected block delimiter");
    reject([] { compile("print \"unfinished"); }, "unterminated string");
    reject([] { compile("print a"); }, "expected string literal");
    reject([] { compile("uint16 a=1 a=(1)"); }, "expression operand");

    for (double value : {-32768.0, -32767.75, -1.5, -1.0, -0.9999999403953552,
            -0.00006103515625, -static_cast<double>(std::numeric_limits<float>::denorm_min()),
            -0.0, 0.0, static_cast<double>(std::numeric_limits<float>::denorm_min()),
            0.5, 1.5, 32766.75, 32767.0}) scalarPair(value);
    scalarPair(-0.00006103515625, -1, 1);
    scalarPair(0.75, -1, 1); // Continuous float interval, not integer sequence.
    scalarPair(-std::numeric_limits<double>::denorm_min(), -1, 1);
    scalarPair(std::numeric_limits<double>::denorm_min(), -1, 1);
    for (double value : {-32768.0, 0.0, 1.0, 32767.0}) scalarPair(value, value, value);
    auto defaulted = termless::compile("// comment\nFLOAT32 X rAnGe(-1,+1) SINT16 Result "
        "sUb Compute Result=SiNt16(FlOoR(X)) eNdSuB");
    assert(defaulted.spec.initial == 0 && defaulted.spec.input == "X");
    const auto valid = scalarSource(0.25, " ", -1, 1);
    auto replace = [&](const std::string &from, const std::string &to) {
        auto text = valid; text.replace(text.find(from), from.size(), to); return text;
    };
    for (const auto &literal : {"1foo", "0x1", "1e", "1e+", "1.2.3"})
        reject([&] { termless::compile(replace("0.25", literal)); }, "malformed numeric literal");
    for (const auto &literal : {"1e9999", "1e-9999"})
        reject([&] { termless::compile(replace("0.25", literal)); }, "representable range");
    for (const auto &literal : {"NaN", "Infinity", "-", "+"})
        reject([&] { termless::compile(replace("0.25", literal)); }, "expected finite decimal number");
    reject([&] { termless::compile(valid + ";"); }, "remove ';'");
    reject([&] { termless::compile(replace("floor(x)", "floor(X)")); }, "declared float32 input");
    reject([&] { termless::compile(replace("result=", "Result=")); }, "declared sint16 output");
    reject([&] { termless::compile(replace("sint16 result", "sint16 x")); }, "duplicate declaration");
    reject([&] { termless::compile(replace("Sub compute", "Sub If")); }, "reserved identifier");
    reject([&] { termless::compile(replace("floor(x)", "floor(x+1)")); }, "expected ')'");
    reject([&] { termless::compile(replace("Range(-1,1)", "Range(-1,1,1)")); }, "expected ')'");
    reject([&] { termless::compile(replace("Range", "Bounds")); }, "expected 'Range'");
    reject([&] { termless::compile(valid + " print \"foo\""); }, "trailing source");
    reject([] { termless::compile(scalarSource(2, " ", -1, 1)); }, "declared Range");
    reject([] { termless::compile(scalarSource(0.1, " ", 0, 0.1)); }, "float32 rounding");
    reject([] { termless::Parser("1 garbage").updateValue(); }, "unexpected text");
    assert(termless::Parser("-.5e+1").updateValue() == -5);
    assert(termless::Parser("+1.").updateValue() == 1);
    assert(termless::Parser("1e-310").updateValue() > 0);

    golem::syntax::Reader spans("// hi\r\n  uint16 x\n= 1");
    auto keyword = spans.pop();
    assert(keyword.begin.offset == 9 && keyword.begin.line == 2 && keyword.begin.column == 3);
    assert(keyword.end.offset == 15 && keyword.end.column == 9);
    assert(spans.pop().text == "x");
    auto equal = spans.pop();
    assert(equal.begin.line == 3 && equal.begin.column == 1);
    reject([] { compile("print \"ok\"\n uint16 a=1foo"); }, "line 2, column 11");
    std::cout << ordinaryPairs << " ordinary byte matches; " << scalarPairs
              << " scalar byte matches; " << rejections << " rejection checks; spans/casing/defaults pass.\n";
}
