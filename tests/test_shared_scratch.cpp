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
    puts("Shared word scratch: three stores use one private triple; default and scope checks pass.");
}
