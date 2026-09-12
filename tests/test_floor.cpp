#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    // Every binary32 integer discontinuity and its immediate neighbours across
    // the accepted domain, including both signs. Away from these discontinuities
    // the floor function is constant. Integer->binary32 is exact in this range.
    for (unsigned n=0;n<=16777215;++n) {
        float at=float(n);
        for (float magnitude : {std::nextafter(at,0.0f),at,std::nextafter(at,INFINITY)}) {
            if (magnitude>16777215) continue;
            float integer=float(uint32_t(magnitude));
            float positive=integer,negative=-(integer+(magnitude!=integer?1.0f:0.0f));
            assert(positive==std::floor(magnitude));
            assert(negative==std::floor(-magnitude));
            assert(std::signbit(negative));
        }
    }
    const std::string base="Matrix source(1100,1,1); Matrix result(1101,1,1); ";
    auto c=hosted::compile(base+"MatValues(source,-0.00006103515625); MatFloor(result,source);");
    assert(c.map.find("negative_fraction_correction")!=std::string::npos);
    (void)hosted::compile(base+"Buffer b(1000,2); Field x(b,0,s16,-1,-1); "
                         "MatValues(source,-0.00006103515625); MatFloor(result,source); StoreI16Floor(x,result);");
    auto reject=[](const std::string &s){bool failed=false;try{(void)hosted::compile(s);}catch(const CompileError&){failed=true;}assert(failed);};
    reject(base+"MatFloor(result,source);");
    reject(base+"MatValues(source,1); MatFloor(source,source);");
    reject(base+"MatValues(source,16777216); MatFloor(result,source);");
    reject(base+"MatValues(source,-16777216); MatFloor(result,source);");
    reject("Matrix source(1100,2,1); Matrix result(1101,1,1); MatValues(source,1,2); MatFloor(result,source);");
    puts("MatFloor host checks and every binary32 integer boundary pass.");
}
