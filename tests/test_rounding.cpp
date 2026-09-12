#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    // Addition is monotone between adjacent half-integer discontinuities.
    // Check each discontinuity and its immediate binary32 neighbours over the
    // entire accepted domain. Both signs share the same magnitude operation.
    for (unsigned n = 0; n < 8388607; ++n) {
        float half = float(n) + 0.5f;
        for (float x : {std::nextafter(half, 0.0f), half, std::nextafter(half, INFINITY)}) {
            float shifted = x + std::nextafter(0.5f, 0.0f);
            assert(std::floor(shifted) == std::round(x));
        }
    }
    const std::string base = "Matrix source(1100,1,1); Matrix result(1101,1,1); ";
    auto result = hosted::compile(base + "MatValues(source,-0.4999999701976776); "
                                         "MatRoundAway(result,source); MatTrunc(source,result);");
    const std::vector<unsigned char> positiveConversion = {41, 8, 0x40};
    assert(std::search(result.code.begin(), result.code.end(), positiveConversion.begin(),
                       positiveConversion.end()) != result.code.end());
    auto reject = [](const std::string &s) {
        bool bad = false;
        try {
            (void)hosted::compile(s);
        } catch (const CompileError &) {
            bad = true;
        }
        assert(bad);
    };
    reject(base + "MatValues(source,16777216); MatTrunc(result,source);");
    reject(base + "MatValues(source,8388608); MatRoundAway(result,source);");
    reject(base + "MatValues(source,1); MatTrunc(source,source);");
    reject(base + "MatTrunc(result,source);");
    reject("Matrix source(1100,2,1); Matrix result(1101,1,1); MatValues(source,1,2); "
           "MatTrunc(result,source);");
    puts("Scalar truncation/rounding: positive conversion golden, five negatives, and every "
         "binary32 half-boundary over the rounded domain pass.");
}
