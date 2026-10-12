#include "../src/resident.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <set>

namespace {
unsigned rejections = 0;
template<class F> void reject(F action, const std::string &message) {
    try { action(); }
    catch (const std::exception &error) {
        if (std::string(error.what()).find(message) == std::string::npos) {
            std::cerr << "Expected " << message << "; got " << error.what() << '\n';
            assert(false);
        }
        ++rejections;
        return;
    }
    std::cerr << "Unexpected acceptance; expected " << message << '\n';
    assert(false && "unexpected acceptance");
}
const std::string floorSource =
    "float32 x Range(-32768,32767)=-0.00006103515625 sint16 result "
    "Sub compute result=sint16(floor(x)) EndSub";
}

int main() {
    using namespace golem;
    assert(ordinary::compile("print \"hello\"") == std::vector<unsigned char>({'h','e','l','l','o'}));
    assert(ordinary::compile("uint16 x") == ordinary::compile("UINT16 x=0"));
    reject([] { ordinary::compile("uint16 x=1 x=X"); }, "undeclared variable");
    reject([] { ordinary::compile("uint16 x;"); }, "remove ';'");
    reject([] { ordinary::compile("For i=1 To 2 Next i"); }, "undeclared variable");
    reject([] { ordinary::compile("uint16 i For i=1 To 2 Next j"); }, "does not match");

    const auto pure = resident::compileResident(floorSource);
    assert(pure.bootstrap.size() == 1008 && pure.owned.size() == 25);
    assert(pure.cleanup.size() == pure.owned.size() * 6 && pure.invoke.size() == 6);
    assert(resident_scalar::inputUpdate(pure, -0.75).size() + pure.invoke.size() == 21);
    const std::string mixed = "uint16 calls=0 uint16 previous=0 "
        "float32 x Range(-32768,32767)=-0.00006103515625 sint16 result "
        "Sub compute previous=calls calls=calls+1 result=sint16(floor(x)) print \"done\\n\" EndSub";
    const auto stateful = resident::compileResident(mixed);
    assert(stateful.extras.errorStatus != 0 && stateful.extras.programs.size() == 1);
    assert(stateful.owned.at(stateful.extras.errorStatus) == 2);
    assert(stateful.cleanup.size() == stateful.owned.size() * 6);
    assert(resident_scalar::inputUpdate(stateful, -0.75).size() + stateful.invoke.size() == 21);
    const std::vector<resident::Reservation> reserved{{1,254},{260,264},{50000,50000}};
    const auto relocated = resident::compileResident(mixed, reserved);
    for (const auto &entry : relocated.owned) {
        assert(entry.first != 0 && entry.first != 65535);
        for (const auto &range : reserved)
            assert(entry.first < range.first || entry.first > range.last);
    }
    assert(relocated.layout.input >= 255 && relocated.extras.globals.size() == 2);
    const auto metadata = resident::manifest(relocated, reserved);
    assert(metadata.find("\"profile\": \"resident-scalar\"") != std::string::npos);
    assert(metadata.find("experimental_task") == std::string::npos);
    assert(metadata.find("\"serialized_nonreentrant\": true") != std::string::npos);
    assert(metadata.find("\"external_input_requires_host_validation\": true") != std::string::npos);
    assert(metadata.find("\"uint16_add_overflow\": 1") != std::string::npos);
    assert(metadata.find("\"reset\": \"each_invocation\"") != std::string::npos);
    assert(pure.extras.errorStatus == 0);
    assert(resident::manifest(pure).find("runtime_error") == std::string::npos);
    assert(resident::compileResident(mixed).bootstrap == stateful.bootstrap);
    for (double value : {-32768.0, -32767.75, -1.5, -1.0, -0.00006103515625,
                         -static_cast<double>(std::numeric_limits<float>::denorm_min()),
                         -0.0, 0.0, static_cast<double>(std::numeric_limits<float>::denorm_min()),
                         0.75, 32766.75, 32767.0})
        assert(resident_scalar::inputUpdate(pure, value).size() == 15);
    for (double value : {-32768.01, 32767.01})
        reject([&] { resident_scalar::inputUpdate(pure, value); }, "declared Range");
    for (double value : {std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()})
        reject([&] { resident_scalar::inputUpdate(pure, value); }, "finite representable float32");

    auto replace = [&](const std::string &from, const std::string &to) {
        auto source = floorSource;
        source.replace(source.find(from), from.size(), to);
        return source;
    };
    reject([&] { resident::compileResident("uint16 x " + floorSource); }, "duplicate declaration");
    reject([&] { resident::compileResident("uint16 a=65535+1 " + floorSource); }, "constant addition overflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a+1 " + floorSource); }, "initializer addition requires constant");
    reject([&] { resident::compileResident("uint16 a=0 " + replace("result=", "a=65535+1 result=")); }, "constant addition overflows");
    assert(resident::compileResident("uint16 a=32768+32767 " + floorSource).extras.errorStatus == 0);
    assert(resident::compileResident("uint16 a=65535-65535+1 " + floorSource).extras.errorStatus == 0);
    reject([&] { resident::compileResident("uint16 a=0-1 " + floorSource); }, "constant subtraction underflows");
    reject([&] { resident::compileResident("uint16 a=0-1+1 " + floorSource); }, "constant subtraction underflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a-1 " + floorSource); }, "initializer subtraction requires constant");
    const auto subtract = resident::compileResident("uint16 a=1 " + replace("result=", "a=a-1 result="));
    assert(subtract.extras.errorStatus && subtract.extras.programs.size() == 2);
    assert(resident::manifest(subtract).find("\"uint16_sub_underflow\": 2") != std::string::npos);
    reject([&] { resident::compileResident("uint16 a=256*256 " + floorSource); }, "constant multiplication overflows");
    reject([&] { resident::compileResident("uint16 a=65535*2*0 " + floorSource); }, "constant multiplication overflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a*1 " + floorSource); }, "initializer multiplication requires constant");
    reject([] { ordinary::compile("uint16 a=2*3"); }, "multiplication requires --resident");
    assert(resident::compileResident("uint16 a=1+2*3-4 " + floorSource).extras.errorStatus == 0);
    assert(resident::compileResident("uint16 a=1+2*3-4 " + floorSource).bootstrap ==
           resident::compileResident("uint16 a=3+0 " + floorSource).bootstrap);
    assert(resident::compileResident("uint16 a=0*65535*65535 " + floorSource).extras.errorStatus == 0);
    const auto multiply = resident::compileResident("uint16 a=2 " + replace("result=", "a=a*a result="));
    assert(resident::manifest(multiply).find("\"uint16_mul_overflow\": 3") != std::string::npos);
    reject([&] { resident::compileResident(replace("floor(x)", "floor(X)")); }, "declared float32 input");
    reject([&] { resident::compileResident(replace("floor(x)", "floor(x+1)")); }, "expected ')'");
    reject([&] { resident::compileResident(replace("Range(-32768,32767)", "Range(-1,1,1)")); }, "expected ')'");
    reject([&] { resident::compileResident(replace("Range(-32768,32767)", "Range(-32769,32767)")); }, "finite input Range");
    reject([&] { resident::compileResident(replace("-0.00006103515625", "32768")); }, "declared Range");
    reject([&] { resident::compileResident(replace("result=sint16(floor(x))", "print \"empty\"")); }, "requires one");
    reject([&] { resident::compileResident(replace("EndSub", "result=sint16(floor(x)) EndSub")); }, "exactly one");
    reject([&] { resident::compileResident(replace("EndSub", "")); }, "expected 'EndSub'");
    reject([&] { resident::compileResident(replace("result=", "For i=1 To 2 Next i result=")); }, "not qualified");
    reject([&] { resident::compileResident(replace("result=", "uint16 local result=")); }, "declare resident uint16 globals");
    reject([&] { resident::compileResident(floorSource + " print \"extra\""); }, "trailing source");
    reject([&] { resident::compileResident(floorSource, {{1,65534}}); }, "namespace exhausted");
    reject([&] { resident::compileResident(floorSource, {{5,10},{8,20}}); }, "already claimed");
    reject([&] { resident::compileResident(floorSource, {{0,1}}); }, "reserved");
    reject([&] { resident::compileResident(floorSource, {{65535,65535}}); }, "reserved");
    reject([] { resident::updateValue("1 garbage"); }, "unexpected text");
    reject([] { resident::updateValue("1e9999"); }, "representable range");
    std::cout << "Maintained raw/resident contract checks pass; " << rejections << " rejection cases.\n";
}
