#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
unsigned ledger(const std::string &map, const std::string &key) {
    auto pos = map.find(key + " "); assert(pos != std::string::npos);
    return unsigned(std::stoul(map.substr(pos + key.size() + 1)));
}
int main() {
    const std::string source =
        "Buffer b(1000,8); Field signedWord(b,0,s16); Field unsignedWord(b,2,u16); "
        "Matrix m(1100,1,1); "
        "Program inner(2001) { MatValues(m,-123.75); StoreI16Floor(signedWord,m); }; "
        "Program outer(2000) { MatValues(m,511.9); StoreU16Floor(unsignedWord,m); "
        "Call(inner); MatValues(m,65534); StoreU16Floor(unsignedWord,m); };",
        prefix = "SharedWordScratch(); ";
    auto isolated = hosted::compile(source), shared = hosted::compile(prefix + source);
    assert(ledger(isolated.map, "owned_ids") - ledger(shared.map, "owned_ids") == 6);
    assert(shared.map.find("scalar_word_scratch shared_nonreentrant_expansion") != std::string::npos);
    assert(isolated.map.find("scalar_word_scratch") == std::string::npos);
    auto reject = [](const std::string &s) {
        bool failed = false;
        try { (void)hosted::compile(s); } catch (const CompileError &) { failed = true; }
        assert(failed);
    };
    reject("SharedWordScratch(1);" + source);
    reject(prefix + prefix + source);
    reject("Buffer b(1000,2); SharedWordScratch();");
    reject("Program p(2000) { SharedWordScratch(); };");
    reject("Repeat(2) { SharedWordScratch(); };");
    const std::string wide =
        "Buffer b(1000,8); Field positive(b,0,s32,0,16777215); "
        "Matrix m(1100,1,1); Matrix r(1101,1,1); "
        "Program leaf(2002) { MatValues(m,-0.25); MatFloor(r,m); }; "
        "Program inner(2001) { MatValues(m,16777215); StorePositive32Floor(positive,m); "
        "Call(leaf); MatValues(m,511.75); MatTrunc(r,m); }; "
        "Program outer(2000) { MatValues(m,-1.5); MatRoundAway(r,m); Call(inner); };";
    auto wideIsolated = hosted::compile(wide), wordOnly = hosted::compile(prefix + wide),
         scalar = hosted::compile("SharedScalarScratch(); " + wide);
    assert(ledger(wideIsolated.map, "owned_ids") == ledger(wordOnly.map, "owned_ids"));
    assert(ledger(wideIsolated.map, "owned_ids") - ledger(scalar.map, "owned_ids") == 9);
    assert(scalar.map.find("scalar_wide_scratch shared_nonreentrant_expansion") != std::string::npos);
    assert(wordOnly.map.find("scalar_wide_scratch") == std::string::npos);
    for (const auto &directive : {std::string("SharedWordScratch"), std::string("SharedScalarScratch")}) {
        reject("SharedScalarScratch(); " + directive + "(); " + source);
        reject(directive + "(); SharedScalarScratch(); " + source);
    }
    reject("SharedScalarScratch(1);" + source);
    reject("Buffer b(1000,2); SharedScalarScratch();");
    reject("Program p(2000) { SharedScalarScratch(); };");
    reject("Repeat(2) { SharedScalarScratch(); };");
    auto scalarWords = hosted::compile("SharedScalarScratch(); " + source);
    assert(ledger(scalarWords.map, "owned_ids") == ledger(shared.map, "owned_ids"));
    assert(ledger(scalarWords.map, "resident_payload_bytes") == ledger(shared.map, "resident_payload_bytes"));
    puts("Shared word scratch: three stores use one private triple; default and scope checks pass.");
}
