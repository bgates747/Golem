#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer src(1500,24); Field a(src,0,s32,-100,100); Field b(src,4,s32,-100,100); Field "
        "c(src,8,s32,-100,100); Field d(src,12,s32,-100,100); Field e(src,16,s32,-100,100); Field "
        "f(src,20,s32,-100,100); Buffer dst(1502,12); Field x(dst,0,s32,-100,100); Field "
        "y(dst,4,s32,-100,100); Field z(dst,8,s32,-100,100); Buffer state(1000,2); Field "
        "index(state,0,u16); Buffer status(1002,2); Field ok(status,0,u16,0,1); ";
    auto result = hosted::compile(base + "Program get(2000) { LoadElement(dst,src,index,ok); }; ");
    assert(result.map.find("source 1500 count 2 stride 12") != std::string::npos);
    assert(result.map.find("source_offset_field 13") != std::string::npos);
    const std::vector<unsigned char> copy = {23, 0,  160, 222, 5, 5, 0xe2, 0,
                                             0,  12, 0,   220, 5, 0, 0};
    assert(std::search(result.code.begin(), result.code.end(), copy.begin(), copy.end()) !=
           result.code.end());
    const std::string numeric = "Buffer b(1000,8); Field narrow(b,0,u16); Field "
                                "wide(b,4,s32,0,65535); Matrix m(1100,1,1); ";
    (void)hosted::compile(
        numeric + "WidenUnsigned(wide,narrow); MatValues(m,511.999847); StoreU16Floor(narrow,m);");
    auto reject = [](const std::string &s) {
        bool bad = false;
        try {
            (void)hosted::compile(s);
        } catch (const CompileError &) {
            bad = true;
        }
        assert(bad);
    };
    reject(base + "LoadElement(src,src,index,ok);");
    reject(base + "LoadElement(dst,src,index,index);");
    reject(base + "LoadElement(dst,src,x,ok);");
    reject(base + "Field alias(src,0,s32); LoadElement(dst,src,index,ok);");
    reject(numeric + "MatValues(m,-0.1); StoreU16Floor(narrow,m);");
    reject(numeric + "MatValues(m,65535); StoreU16Floor(narrow,m);");
    reject(numeric + "StoreU16Floor(narrow,m);");
    reject(numeric + "WidenUnsigned(narrow,wide);");
    reject("Buffer b(1000,4); Field s(b,0,u16); Field d(b,2,s16); WidenUnsigned(d,s);");
    reject("Buffer b(1000,4); Field s(b,0,u16); Field d(b,0,s32); WidenUnsigned(d,s);");
    reject("Matrix m(1100,1,1); Field cell(m,0,f32,0,1); Buffer b(1000,4); Field x(b,0,f32,0,1); "
           "MatValues(m,50); Copy(x,cell,4);");
    puts("Element helpers: typed record selection, offset relocation, unsigned floor/widening and "
         "ten negatives pass.");
}
