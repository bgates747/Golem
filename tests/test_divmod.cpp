#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer input(1000,4); Field x(input,0,s32,0,3276799); Buffer output(1001,4); Field "
        "q(output,0,u16,0,511); Field r(output,2,u16,0,6399); ";
    auto result = hosted::compile(base + "Program p(2000) { DivModPositive(q,r,x,6400); }; ");
    assert(result.map.find("divisor 6400 exhaustive_domain 0 3276799") != std::string::npos);
    // MatScale with a fetched scalar requires 0x20 in the operation byte.
    const std::vector<unsigned char> fetchedScale = {34, 0x27, 1, 1};
    assert(std::search(result.code.begin(), result.code.end(), fetchedScale.begin(),
                       fetchedScale.end()) != result.code.end());
    auto reject = [](const std::string &s) {
        bool bad = false;
        try {
            (void)hosted::compile(s);
        } catch (const CompileError &) {
            bad = true;
        }
        assert(bad);
    };
    reject(base + "DivModPositive(q,r,x,0);");
    reject(base + "DivModPositive(q,r,x,1);");
    reject(base + "DivModPositive(q,r,x,6401);");
    reject(base + "DivModPositive(q,q,x,6400);");
    reject("Buffer input(1000,4); Field x(input,0,s32,-1,100); Buffer output(1001,4); Field "
           "q(output,0,u16); Field r(output,2,u16); DivModPositive(q,r,x,10);");
    reject("Buffer input(1000,4); Field x(input,0,s32,0,16777216); Buffer output(1001,4); Field "
           "q(output,0,u16); Field r(output,2,u16); DivModPositive(q,r,x,6400);");
    // Constant division is not safe for every binary32 domain: scan must reject
    // where the last fraction rounds up to the next integer at larger progress.
    reject("Buffer input(1000,4); Field x(input,0,s32,0,16777215); Buffer output(1001,4); Field "
           "q(output,0,u16); Field r(output,2,u16); DivModPositive(q,r,x,4000);");
    reject("Matrix m(1100,3,3); Identity(m); Scale(m,1e-50,1); InvertAffine(m);");
    reject("Buffer b(1000,4); Field tiny(b,0,f32,1e-50,1e-50); Set(tiny,1e-50);");
    puts("DivModPositive: exhaustive declared domain, fetched-scalar golden, alias/range/rounding "
         "rejection and nine negatives pass.");
}
