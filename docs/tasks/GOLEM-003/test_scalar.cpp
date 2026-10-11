#include "scalar_frontend.hpp"
#define main rally_oracle_cli_main
#include "reference/src/golemc.cpp"
#undef main
#include <cassert>

std::string natural(double value, double lo = -32768, double hi = 32767) {
    std::ostringstream out;
    out << std::setprecision(17) << "float32 x Range(" << lo << ',' << hi << ")=" << value
        << "; sint16 result; Sub compute; result=sint16(floor(x)); EndSub;";
    return out.str();
}
std::string rally(double value, double lo = -32768, double hi = 32767) {
    std::ostringstream out;
    out << std::setprecision(17)
        << "Buffer input(1000,4); Field x(input,0,f32," << lo << ',' << hi << "); Set(x," << value
        << "); Buffer output(1001,2); Field result(output,0,s16); Matrix source(1100,1,1); "
        << "Matrix integral(1101,1,1); Program compute(2000) { MatLoad(source,x); "
        << "MatFloor(integral,source); StoreI16Floor(result,integral); };";
    return out.str();
}
template<typename F> void reject(F action) {
    bool failed = false;
    try { action(); } catch (const scalar::Error &) { failed = true; }
    assert(failed);
}
void equivalent(double value, double lo = -32768, double hi = 32767) {
    auto result = readable::compile(natural(value, lo, hi));
    auto oracle = hosted::compile(rally(value, lo, hi));
    assert(result.bootstrap == oracle.code);
    assert(result.cleanup == oracle.clear);
}
int main() {
    // Compare the complete installation stream and owned cleanup, not selected
    // opcodes alone. Every helper, scratch allocation and constant must match.
    const std::vector<double> values = {-32768, -32767.75, -1.5, -1, -0.9999999403953552,
        -0.00006103515625, -std::numeric_limits<float>::denorm_min(), -0.0,
        0.0, std::numeric_limits<float>::denorm_min(), 0.5, 1.5, 32766.75, 32767};
    for (double value : values) equivalent(value);
    equivalent(-0.00006103515625, -1, 1);
    equivalent(1, 1, 1);
    equivalent(-32768, -32768, -32768);
    equivalent(0, 0, 0);
    equivalent(32767, 32767, 32767);
    auto fixture = readable::compile(readFile("examples/floor.golem"));
    auto oracle = hosted::compile(readFile("examples/floor-rally.golem"));
    assert(fixture.bootstrap == oracle.code && fixture.cleanup == oracle.clear);

    auto mixed = readable::compile("// comment\nFLOAT32 X rAnGe(-1,1); SINT16 Result; "
        "sUb Compute; Result = SiNt16(FlOoR(X)); eNdSuB;");
    assert(mixed.spec.initial == 0 && mixed.spec.input == "X");
    assert(mixed.programs == fixture.programs); // Names carry no platform IDs.
    for (const auto &text : {
        "float32 x Range(-1,1); sint16 y; Sub run; y=sint16(floor(X)); EndSub;",
        "float32 x Range(-1,1); sint16 y; Sub run; Y=sint16(floor(x)); EndSub;",
        "float32 Floor Range(-1,1); sint16 y; Sub run; y=sint16(floor(Floor)); EndSub;",
        "float32 x Range(-1,1); sint16 x; Sub run; x=sint16(floor(x)); EndSub;",
        "float32 x Range(-1,1); sint16 y; Sub x; y=sint16(floor(x)); EndSub;",
        "float32 x Range(-1,1); sint16 y; Sub run; y=floor(x); EndSub;",
        "float32 x Range(-1,1); sint16 y; Sub run; y=sint16(floor(x+1)); EndSub;",
        "float32 x Range(-1,1); sint16 y; Sub run; y=sint16(floor(x)); EndSub",
        "float32 x Range(-1,1); sint16 y; Sub run; y=sint16(floor(x)); EndSub; trailing",
        "float32 x Range(-1,1) = 1e; sint16 y; Sub run; y=sint16(floor(x)); EndSub;",
        "float32 x Range(-1,1) = NaN; sint16 y; Sub run; y=sint16(floor(x)); EndSub;",
        "float32 x Range(-1,1) = 0x0; sint16 y; Sub run; y=sint16(floor(x)); EndSub;"
    }) reject([&] { (void)readable::compile(text); });
    reject([] { (void)readable::compile(natural(0, 1, -1)); });
    reject([] { (void)readable::compile(natural(0, -32769, 32767)); });
    reject([] { (void)readable::compile(natural(0, -32768, 32768)); });
    reject([] { (void)readable::compile(natural(2, -1, 1)); });
    reject([] { (void)readable::compile(natural(0.1, 0, 0.1)); }); // binary32 rounds above bound.
    reject([] { (void)readable::Parser("1 garbage").updateValue(); });

    // Updates change the input only, not scratch, result, code or install stream.
    auto saved = fixture.bootstrap;
    for (double value : values) {
        auto update = scalar::inputUpdate(fixture, value);
        assert(update.size() == 15 && update[5] == 5 && update[6] == 0xc2);
        assert(std::equal(update.end() - 4, update.end(), scalar::binary32(value).begin()));
    }
    assert(fixture.bootstrap == saved && fixture.invoke == scalar::Bytes({23,0,160,208,7,1}));
    for (double value : {32768.0, -32769.0, std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
        reject([&] { (void)scalar::inputUpdate(fixture, value); });
    reject([&] { (void)scalar::inputUpdate(mixed, 1.01); });
    auto collision = fixture.layout; collision.output = collision.input;
    reject([&] { (void)scalar::compile(fixture.spec, collision); });
    puts("Scalar slice: 20 full Rally oracle matches; syntax/range/rounding rejections; bounded updates pass.");
}
