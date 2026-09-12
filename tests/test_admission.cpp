#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer state(1000,8); Field value(state,0,s32,-38400,38400); Field seq(state,4,u16); "
        "Field duplicate(state,6,u16); Buffer status(1002,2); Field ok(status,0,u16,0,1); ";
    auto result = hosted::compile(base + "Program admit(2000) { Set(ok,1); RequireRange(value,ok); "
                                         "RequireEqual(seq,duplicate,ok); }; ");
    assert(result.map.find("require_range 2000 1000:0 s32 -38400 38400 sticky_status 1002:0") !=
           std::string::npos);
    assert(result.map.find("require_equal 2000 1000:4 1000:6 sticky_status 1002:0") !=
           std::string::npos);
    assert(result.map.find("program_payload_bytes") != std::string::npos);
    // All validation comparisons fetch zero-extended words, with both operands
    // buffer-backed. The normalization's only sign operation is byte XOR 0x80.
    const std::vector<unsigned char> signXor = {5, 0xc7, 6, 0, 1, 0, 128};
    assert(std::search(result.code.begin(), result.code.end(), signXor.begin(), signXor.end()) !=
           result.code.end());
    const std::string records =
        "Buffer src(1000,4); Field a(src,0,s16,-10,10); Field b(src,2,u16,0,100); Buffer "
        "dst(1001,4); Field c(dst,0,s16,-20,20); Field d(dst,2,u16); ";
    auto record = hosted::compile(records + "Program copy(2000) { CopyRecord(dst,src); };");
    const std::vector<unsigned char> copy = {23, 0, 160, 233, 3, 5, 0xe2, 0, 0, 4, 0, 232, 3, 0, 0};
    assert(std::search(record.code.begin(), record.code.end(), copy.begin(), copy.end()) !=
           record.code.end());
    auto reject = [](const std::string &s) {
        bool failed = false;
        try {
            (void)hosted::compile(s);
        } catch (const CompileError &) {
            failed = true;
        }
        assert(failed);
    };
    reject(base + "RequireRange(value,2,-2,ok);");
    reject(base + "RequireRange(value,-2147483649,0,ok);");
    reject(base + "RequireRange(value,seq);");
    reject(base + "RequireRange(value,0,1);");
    reject(base + "RequireEqual(value,seq,ok);");
    reject(base + "RequireEqual(seq,duplicate,seq);");
    reject("Buffer b(1000,4); Field v(b,0,u16); Field s(b,2,u16,0,1); RequireRange(v,s);");
    reject("Buffer b(1000,4); Field v(b,0,f32); Buffer status(1002,2); Field ok(status,0,u16,0,1); "
           "RequireRange(v,0,1,ok);");
    reject("Buffer b(1000,4); Field v(b,0,q8s32); Buffer status(1002,2); Field "
           "ok(status,0,u16,0,1); RequireRange(v,0.0001,0.0002,ok);");
    reject(records + "CopyRecord(src,dst);");
    reject(records + "CopyRecord(src,src);");
    reject(records + "Field alias(src,0,s16); CopyRecord(dst,src);");
    reject("Buffer src(1000,4); Field a(src,0,s16); Buffer dst(1001,4); Field b(dst,0,s16); "
           "CopyRecord(dst,src);");
    reject("Buffer src(1000,4); Buffer dst(1001,4); CopyRecord(dst,src);");
    reject("Buffer b(1000,65535); Buffer more(1001,2);");
    reject("Buffer b(1000,2); Field f(\"b\",0,u16);");
    reject("Buffer b(1000,2); Field f(b,0,\"u16\");");
    std::string deep = "Program p(2000) { ";
    for (unsigned i = 0; i < 64; ++i)
        deep += "Repeat(1) { ";
    for (unsigned i = 0; i < 65; ++i)
        deep += "}; ";
    reject(deep);
    std::string programs;
    for (unsigned i = 0; i < 3; ++i)
        programs += "Program p" + std::to_string(i) + "(" + std::to_string(2000 + i) +
                    ") { print \"" + std::string(50000, 'x') + "\"; }; ";
    reject(programs);
    puts("Admission compiler: signed-byte normalization, sticky guards, typed record copy, "
         "resource caps, syntax depth and 19 negatives pass.");
}
