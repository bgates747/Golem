// Maintained promotion contracts; frozen fixture provenance is under portable/fixtures.
#include "../src/portable/driver.hpp"
#include <cassert>
#include <iostream>
#include <iterator>

namespace fs = std::filesystem;
std::string read(const fs::path &path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
void write(const fs::path &path, const std::string &text) {
    std::ofstream out(path, std::ios::binary); out << text; assert(out.good());
}
int invoke(std::vector<std::string> args) {
    std::vector<char *> argv;
    for (auto &arg : args) argv.push_back(arg.data());
    std::ostringstream sink;
    auto *out = std::cout.rdbuf(sink.rdbuf()); auto *err = std::cerr.rdbuf(sink.rdbuf());
    const int rc = golem::portable_cli::run(static_cast<int>(argv.size()), argv.data());
    std::cout.rdbuf(out); std::cerr.rdbuf(err);
    return rc;
}
int main(int argc, char **argv) {
    assert(argc == 3);
    const fs::path fixtures = argv[1], work = argv[2];
    fs::create_directories(work);
    unsigned files = 0, nativeSources = 0, rejected = 0;
    for (const char *name : {"default", "relocated", "animation"}) {
        std::vector<std::string> args{"golemc", "--portable"};
        if (std::string(name) == "relocated") args.insert(args.end(), {"--reserve", "1:254"});
        const fs::path output = work / name;
        args.insert(args.end(), {(fixtures / name / "source.golem").string(), output.string()});
        assert(invoke(args) == 0);
        for (const auto &expected : fs::directory_iterator(fixtures / name / "expected")) {
            // Frozen native C is preserved; executable release-failure tests
            // qualify its intentional change rather than rewriting the baseline.
            if (expected.path().filename() == "native.c") {
                assert(fs::exists(output / "native.c")); ++nativeSources; continue;
            }
            assert(read(expected.path()) == read(output / expected.path().filename())); ++files;
        }
    }
    const fs::path source = work / "source.golem", output = work / "rejected";
    write(source, "uint16 x=0 Sub step x=x+1 EndSub");
    auto bad = [&](std::vector<std::string> options) {
        std::vector<std::string> args{"golemc", "--portable"};
        args.insert(args.end(), options.begin(), options.end());
        args.insert(args.end(), {source.string(), output.string()});
        assert(invoke(args) == 1); assert(!fs::exists(output)); ++rejected;
    };
    for (const char *range : {"0:1", "1:65535", "2:1", "1:2junk", "-1:2", "1:2:3",
                              "1:", ":2", "60000:60001", "99999999999999999999999:2"})
        bad({"--reserve", range});
    bad({"--reserve", "1:10", "--reserve", "10:20"});
    bad({"--bind", "1", "8", "0", "2", "--reserve", "8:8"});
    for (const char *role : {"0", "5", "1junk", "-1", "99999999999999999999"})
        bad({"--bind", role, "8", "0", "2"});
    bad({"--bind", "1", "0", "0", "2"});
    bad({"--bind", "1", "8", "4294967295", "1"});
    bad({"--bind", "1", "8", "4294967296", "1"});
    bad({"--bind", "1", "8", "0", "0"});
    bad({"--bind", "1", "8", "0", "2", "--bind", "1", "9", "0", "2"});
    bad({"--bind", "1", "8", "0", "2", "--bind", "2", "8", "0", "2"});
    bad({"--target", "native"}); bad({"--array-data", "unknown=missing"});
    bad({"--array-data", "x="}); bad({"--array-data", "=file"});
    bad({"--array-data", "x=first", "--array-data", "x=second"});
    assert(invoke({"golemc", "--portable"}) == 1);
    assert(invoke({"golemc", "--portable", "--reserve"}) == 1);
    assert(invoke({"golemc", "--portable", "--bind", "1", "2"}) == 1);

    const fs::path packed = work / "data.dat";
    write(packed, std::string("\x34\x12\x78\x56", 4));
    write(source, "uint16 index=1 uint16 value=0 ReadOnly Array uint16 data(-1:0) "
                  "Sub step value=data(sint16(index)-1) EndSub");
    assert(invoke({"golemc", "--portable", "--reserve", "1:9", "--reserve", "20:22",
                   "--bind", "2", "500", "1", "4", "--array-data", "data=" + packed.string(),
                   source.string(), (work / "mapped").string()}) == 0);
    assert(read(work / "mapped/array-data.bin") == read(packed));
    write(packed, "short"); bad({"--array-data", "data=" + packed.string()});
    write(source, "uint16 v=0 Array uint16 data(0:0)=Fill(1) Sub step v=data(0) EndSub");
    bad({"--array-data", "data=" + packed.string()});

    // Independent expected reference results for combined types/control/storage.
    const std::string sample =
        "uint16 i=0 uint16 total=0 uint32 wide=4294967295 sint32 negative=-65 "
        "uint16 bits=6 sint32 shifted=0 float32 x=1.5 sint16 trunc=0 bool flag=false "
        "Array uint16 values(2:4)=Values(1,2,3) "
        "Sub step For i In Range(2:4) total=total+values(i) Next i "
        "shifted=negative >> bits trunc=sint16(x) "
        "flag=Not false And total==6 Or false If flag Then x=x*2.0 Else x=0.0 EndIf EndSub";
    auto program = portable::Parser(sample).parse();
    auto layout = portable::layout(program);
    auto state = layout.initial;
    portable::ArrayState arrays{{layout.arrayBuffers[0], program.arrays[0].initial}};
    assert(portable::invokeReference(program, layout, state, 0, &arrays) == 0);
    auto value = [&](size_t index) {return portable::get(state, layout.offsets[index], program.globals[index].type);};
    assert(value(0).bits == 4 && value(1).bits == 6 && value(5).number() == -2);
    assert(value(6).number() == 3.0 && value(7).number() == 1 && value(8).bits == 1);
    for (const char *text : {"uint16 x Sub f x=1 EndSub", "uint16 x=0; Sub f x=1 EndSub",
                            "uint16 x=0 Sub f x=x >> 1 EndSub",
                            "uint32 x=1 Sub f x=x >>32 EndSub",
                            "uint16 x=0 Sub f x=true EndSub"}) {
        bool failed = false;
        try { portable::Parser(text).parse(); } catch (const std::exception &) { failed = true; }
        assert(failed); ++rejected;
    }
    std::cout << "Portable contracts pass: " << files << " frozen output files, "
              << nativeSources << " intentionally changed native sources, " << rejected
              << " rejections, mapped/reserved/bound inputs and combined reference behavior.\n";
}
