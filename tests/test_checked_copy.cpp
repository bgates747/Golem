#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer input(1000,2); Field raw(input,0,u16); "
        "Buffer output(1001,2); Field depth(output,0,u16,64,1100); "
        "Buffer flags(1002,2); Field ok(flags,0,u16,0,1); Set(depth,64); ";
    auto result = hosted::compile(base + "Program p(2000) { CopyChecked(depth,raw,ok); };");
    assert(result.map.find("copy_checked 2000 1000:0 to 1001:0 u16 64 1100 status 1002:0") !=
           std::string::npos);
    auto reject = [](const std::string &s) {
        bool failed = false;
        try { (void)hosted::compile(s); } catch (const CompileError &) { failed = true; }
        assert(failed);
    };
    reject(base + "Copy(depth,raw,2);"); // unchecked narrowing remains prohibited
    reject(base + "CopyChecked(depth,raw,raw);");
    reject(base + "CopyChecked(depth,raw,depth);");
    reject(base + "CopyChecked(depth,depth,ok);");
    reject("Buffer a(1000,4); Field x(a,0,f32); Buffer b(1001,4); Field y(b,0,f32); "
           "Buffer s(1002,2); Field ok(s,0,u16,0,1); CopyChecked(y,x,ok);");
    reject("Buffer a(1000,4); Field x(a,0,u16); Buffer b(1001,4); Field y(b,0,s16); "
           "Buffer s(1002,2); Field ok(s,0,u16,0,1); CopyChecked(y,x,ok);");
    reject("Buffer a(1000,2); Field x(a,0,u16); Buffer b(1001,2); Field y(b,0,u16); "
           "Buffer s(1002,2); Field ok(s,0,u16); CopyChecked(y,x,ok);");
    for (const auto *type : {"u8", "u16", "s16", "s32", "q8s16", "q8s32"}) {
        std::string source = "Buffer a(1000,4); Field x(a,0," + std::string(type) + "); "
                             "Buffer b(1001,4); Field y(b,0," + std::string(type) + ",1,10); "
                             "Buffer f(1002,2); Field flag(f,0,u16,0,1); Set(y,1); CopyChecked(y,x,flag);";
        (void)hosted::compile(source);
    }
    puts("Checked copy host tests: guarded narrowing, fixed formats and alias/type negatives pass.");
}
