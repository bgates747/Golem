#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer in(1000,16); Field x(in,0,s32,0,6553599); Field a(in,4,s32); "
        "Field b(in,8,s32); Buffer out(1001,16); Field q(out,0,u16,0,1); "
        "Field r(out,4,s32,0,3276799); Field less(out,8,u16,0,1); ";
    auto large = hosted::compile(base + "Program p(2000) { DivModPositive(q,r,x,3276800); "
                                         "LessThan(less,a,b); };");
    assert(large.map.find("divisor 3276800 exhaustive_domain 0 6553599") != std::string::npos);
    assert(large.map.find("less_than 2000 s32 1000:4 1000:8 result 1001:8 ordered_bytes 4") !=
           std::string::npos);
    const std::vector<unsigned char> positive32 = {41, 8, 0x40};
    assert(std::search(large.code.begin(), large.code.end(), positive32.begin(), positive32.end()) !=
           large.code.end());
    auto reject = [](const std::string &s) {
        bool failed = false;
        try { (void)hosted::compile(s); } catch (const CompileError &) { failed = true; }
        assert(failed);
    };
    const std::string scalar =
        "Buffer in(1000,4); Field x(in,0,f32,-16777215,16777215); "
        "Buffer out(1001,4); Field result(out,0,s32,0,16777215); "
        "Matrix src(1100,1,1); Matrix dst(1101,1,1); ";
    (void)hosted::compile(scalar + "MatLoad(src,x); MatAbs(dst,src); StorePositive32Floor(result,dst);");
    reject(scalar + "MatLoad(src,x); StorePositive32Floor(result,src);");
    reject(scalar + "MatValues(src,16777216); StorePositive32Floor(result,src);");
    reject(scalar + "StorePositive32Floor(result,src);");
    reject(scalar + "MatValues(src,1); MatAbs(src,src);");
    reject(scalar + "MatAbs(dst,src);");
    reject(scalar + "Matrix wrong(1200,3,3); Identity(wrong); MatAbs(dst,wrong);");
    reject("Buffer b(1000,4); Field x(b,0,s32,0,10); Matrix m(1100,1,1); "
           "MatValues(m,11); StorePositive32Floor(x,m);");
    reject("Buffer b(1000,4); Field x(b,0,u16); Matrix m(1100,1,1); "
           "MatValues(m,11); StorePositive32Floor(x,m);");
    reject(base + "DivModPositive(q,r,x,16777216);");
    reject(base + "DivModPositive(q,r,x,3276801);");
    reject(base + "DivModPositive(q,x,x,3276800);");
    (void)hosted::compile(base + "LessThan(q,a,b); LessThan(less,a,a);");
    reject(base + "LessThan(r,a,b);");
    reject(base + "LessThan(less,less,less);");
    reject(scalar + "Buffer flags(1200,2); Field flag(flags,0,u16,0,1); LessThan(flag,x,x);");
    reject("Buffer b(1000,8); Field x(b,0,s16); Field y(b,2,u16); "
           "Field flag(b,4,u16,0,1); LessThan(flag,x,y);");
    for (const auto *type : {"u8", "u16", "s16", "s32", "q8s16", "q8s32"}) {
        std::string source = "Buffer a(1000,4); Field x(a,0," + std::string(type) + "); "
                             "Buffer b(1001,4); Field y(b,0," + std::string(type) + "); "
                             "Buffer f(1002,2); Field flag(f,0,u16,0,1); LessThan(flag,x,y);";
        (void)hosted::compile(source);
    }
    puts("Vehicle math host checks pass.");
}
