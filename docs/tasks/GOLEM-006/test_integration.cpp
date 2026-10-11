#include "integration.hpp"
#include "../GOLEM-003/scalar_frontend.hpp"
#include <cassert>
#include <filesystem>

namespace {
unsigned rejected = 0, matches = 0;
template<class F> void reject(F action, const std::string &message) {
    try { action(); }
    catch (const std::exception &error) {
        if (std::string(error.what()).find(message) == std::string::npos) {
            std::cerr << "Expected: " << message << "; actual: " << error.what() << '\n';
            assert(false);
        }
        ++rejected; return;
    }
    assert(false && "unexpected acceptance");
}
void dump(const std::string &name, const integrated::Result &result,
          const std::vector<integrated::Reservation> &reservations = {}) {
    auto dir = std::filesystem::path("build/fixtures") / name;
    std::filesystem::create_directories(dir);
    auto write = [&](const char *file, const resident_scalar::Bytes &bytes) {
        std::ofstream out(dir / file, std::ios::binary);
        out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        assert(out.good());
    };
    write("bootstrap.bin", result.bootstrap); write("cleanup.bin", result.cleanup); write("invoke.bin", result.invoke);
    auto update = resident_scalar::inputUpdate(result, -0.75);
    update.insert(update.end(), result.invoke.begin(), result.invoke.end());
    write("update-call.bin", update);
    std::ofstream(dir / "interface.json") << integrated::manifest(result, reservations);
}
void equivalent(double value, double lo = -32768, double hi = 32767) {
    scalar::Spec oldSpec{"x", "result", "compute", {lo, hi}, value};
    resident_scalar::Spec spec{"x", "result", "compute", {lo, hi}, value};
    auto old = scalar::compile(oldSpec);
    golem::BufferAllocator allocation(2);
    auto next = resident_scalar::compile(spec, allocation, resident_scalar::Policy::RallyCompatibility);
    assert(old.bootstrap == next.bootstrap && old.cleanup == next.cleanup && old.invoke == next.invoke);
    assert(old.initial == next.initial && old.programs == next.programs);
    assert(scalar::inputUpdate(old, value) == resident_scalar::inputUpdate(next, value));
    ++matches;
}
}
int main() {
    golem::BufferAllocator allocation;
    allocation.reserve(3, 5, "asset");
    assert(allocation.allocate("a") == 1 && allocation.allocate("b") == 2 && allocation.allocate("c") == 6);
    reject([&] { allocation.claim(3, "collision"); }, "already claimed by asset");
    reject([&] { allocation.reserve(7, 8, "late"); }, "must precede");
    golem::BufferAllocator overlap;
    overlap.reserve(5, 7, "first");
    const auto count = overlap.claims().size();
    reject([&] { overlap.reserve(3, 6, "overlap"); }, "already claimed");
    assert(overlap.claims().size() == count && !overlap.claims().count(3));
    reject([&] { overlap.reserve(8, 6, "reversed"); }, "reversed");
    for (unsigned id : {0u, 65535u, 65536u, std::numeric_limits<unsigned>::max()}) {
        golem::BufferAllocator invalid;
        reject([&] { invalid.claim(id, "invalid"); }, "reserved");
        reject([&] { invalid.reserve(id, id, "invalid"); }, "reserved");
    }
    reject([] { golem::BufferAllocator invalid(2, 1); }, "reversed");
    golem::BufferAllocator last(65534, 65534);
    assert(last.allocate("last") == 65534);
    reject([&] { last.allocate("overflow"); }, "namespace exhausted");
    golem::BufferAllocator full;
    full.reserve(1, 65534, "all assets");
    reject([&] { full.allocate("overflow"); }, "namespace exhausted");
    golem::BufferAllocator small(65533, 65534);
    Compiler ordinary(small);
    assert(ordinary.accumulatorId() == 65533);
    assert(ordinary.declareVariable("x", {}) == 65534);
    reject([&] { ordinary.declareVariable("y", {0, 2, 1}); }, "line 2, column 1: buffer ID namespace exhausted");
    for (double value : {-32768.0, -32767.75, -1.5, -1.0, -0.9999999403953552,
            -0.00006103515625, -static_cast<double>(std::numeric_limits<float>::denorm_min()),
            -0.0, 0.0, static_cast<double>(std::numeric_limits<float>::denorm_min()),
            0.5, 1.5, 32766.75, 32767.0}) equivalent(value);
    equivalent(-0.00006103515625, -1, 1); equivalent(0.75, -1, 1);
    for (double value : {-32768.0, 0.0, 1.0, 32767.0}) equivalent(value, value, value);
    equivalent(-std::numeric_limits<double>::denorm_min(), -1, 1);
    equivalent(std::numeric_limits<double>::denorm_min(), -1, 1);
    resident_scalar::Spec spec{"x", "result", "compute", {-32768, 32767}, -0.00006103515625};
    golem::BufferAllocator compatibility(2);
    auto compat = resident_scalar::compile(spec, compatibility, resident_scalar::Policy::RallyCompatibility);
    dump("compat", compat);
    golem::BufferAllocator conflict(2);
    conflict.reserve(1000, 1001, "interface assets");
    reject([&] { resident_scalar::compile(spec, conflict, resident_scalar::Policy::RallyCompatibility); }, "already claimed");
    golem::BufferAllocator scarce(1, 20);
    reject([&] { resident_scalar::compile(spec, scarce); }, "namespace exhausted");

    auto floor = readFile("examples/floor.golem");
    auto mixed = readFile("examples/counted-floor.golem");
    dump("floor-default", integrated::compileResident(floor));
    const std::vector<integrated::Reservation> ranges{{1, 8}, {20, 24}, {1000, 1200}, {2000, 2000}};
    dump("floor-reserved", integrated::compileResident(floor, ranges), ranges);
    dump("mixed-default", integrated::compileResident(mixed));
    dump("mixed-reserved", integrated::compileResident(mixed, ranges), ranges);
    auto initialized = std::string("uint16 seed=5 uint16 calls=seed+2 ") + floor;
    initialized.insert(initialized.find("result ="), "calls=calls+1 ");
    dump("initialized-expressions", integrated::compileResident(initialized));
    auto wrapping = std::string("uint16 calls=65534 ") + floor;
    wrapping.insert(wrapping.find("result ="), "calls=calls+1 ");
    dump("wrapping-counter", integrated::compileResident(wrapping));
    auto replace = [&](const std::string &from, const std::string &to) {
        auto text = floor; text.replace(text.find(from), from.size(), to); return text;
    };
    reject([&] { integrated::compileResident("uint16 x " + floor); }, "duplicate declaration");
    reject([&] { integrated::compileResident("uint16 result " + floor); }, "duplicate declaration");
    reject([&] { integrated::compileResident("uint16 compute " + floor); }, "duplicate declaration");
    reject([&] { integrated::compileResident(replace("floor(x)", "floor(X)")); }, "declared float32 input");
    reject([&] { integrated::compileResident(replace("result =", "result=1 result =")); }, "expected 'sint16'");
    reject([&] { integrated::compileResident(replace("EndSub", "result=sint16(floor(x)) EndSub")); }, "exactly one");
    reject([&] { integrated::compileResident(replace("result = sint16(floor(x))", "print \"empty\"")); }, "requires one");
    reject([&] { integrated::compileResident(replace("result =", "For i=1 To 2 Next i result =")); }, "not qualified");
    reject([&] { integrated::compileResident(replace("result =", "uint16 y result =")); }, "declare resident uint16 globals");
    reject([&] { integrated::compileResident(replace("result =", "missing=1 result =")); }, "undeclared variable");
    reject([&] { integrated::compileResident(replace("EndSub", "")); }, "expected 'EndSub'");
    reject([&] { integrated::compileResident(floor + " Sub another EndSub"); }, "one job");
    reject([&] { integrated::compileResident(floor, {{1, 65534}}); }, "namespace exhausted");
    reject([&] { integrated::compileResident(floor, {{10, 20}, {15, 30}}); }, "already claimed");
    reject([&] { integrated::compileResident(replace("-0.00006103515625", "32768")); }, "declared Range");
    reject([&] { integrated::compileResident(replace("-0.00006103515625", "1e9999")); }, "representable range");
    std::cout << matches << " full compatibility matches; " << rejected
              << " allocation/integration rejections; 7 decoder fixtures written.\n";
}
