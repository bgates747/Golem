#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
using B = std::vector<unsigned char>;
unsigned word(const B &v, size_t i) { return v.at(i) | (unsigned(v.at(i + 1)) << 8); }
std::map<unsigned, B> uploads(const hosted::Result &r) {
    const auto &v = r.code;
    assert(B(v.begin(), v.begin() + 7) == B({23, 0, 248, 1, 0, 1, 0}));
    std::map<unsigned, B> result;
    for (size_t p = 7; p + 14 <= v.size() && v[p] == 23 && v[p + 2] == 160 && v[p + 5] == 2;) {
        unsigned id = word(v, p + 3);
        p += 6;
        assert(v[p] == 23 && v[p + 2] == 160 && word(v, p + 3) == id && v[p + 5] == 0);
        unsigned size = word(v, p + 6);
        p += 8;
        assert(p + size <= v.size());
        result[id] = B(v.begin() + p, v.begin() + p + size);
        p += size;
    }
    return result;
}
void rejects(const std::string &s) {
    bool caught = false;
    try {
        (void)hosted::compile(s);
    } catch (const CompileError &) {
        caught = true;
    }
    assert(caught);
}
int main() {
    const std::string decl = "Buffer data(1000,2); Field x(data,0,u16,0,10); ";
    auto plain = hosted::compile(decl +
        "Program a(2000) { Set(x,1); Call(b); Set(x,2); Call(b); }; "
        "Program b(2001) { Set(x,3); Call(c); }; Program c(2002) { Set(x,4); }; ");
    auto expanded = hosted::compile(decl +
        "InlineCalls(b,c); Program a(2000) { Set(x,1); Call(b); Set(x,2); Call(b); }; "
        "Program b(2001) { Set(x,3); Call(c); }; Program c(2002) { Set(x,4); }; ");
    auto reference = hosted::compile(decl +
        "Program a(2000) { Set(x,1); Set(x,3); Set(x,4); Set(x,2); Set(x,3); Set(x,4); }; "
        "Program b(2001) { Set(x,3); Set(x,4); }; Program c(2002) { Set(x,4); }; ");
    auto e = uploads(expanded), r = uploads(reference);
    assert(e == r); // Same whole program/data image as explicit source expansion.
    assert(expanded.clear == plain.clear); // Ownership and cleanup preserved.
    assert(expanded.map.find("inline_call 2000") != std::string::npos);
    auto cond = uploads(hosted::compile(decl +
        "InlineCalls(b); Program a(2000) { CallIf(b,x,eq,x); }; "
        "Program b(2001) { Set(x,2); }; "));
    assert(cond[2000].size() == 15 && cond[2000][5] == 6); // No conditional rewrite.
    auto loop = uploads(hosted::compile(decl +
        "InlineCalls(b); Program a(2000) { Repeat(3) { Call(b); }; }; "
        "Program b(2001) { Set(x,2); }; "));
    assert(loop[4].size() == loop[2001].size() + 28); // finite tail loop retained.
    rejects("InlineCalls();");
    rejects("InlineCalls(missing);");
    rejects("Buffer b(1000,2); InlineCalls(b);");
    rejects("Program b(2000) { InlineCalls(b); };");
    rejects("InlineCalls(a,b); Program a(2000) { Call(b); }; Program b(2001) { Call(a); };");
    // Exponential source expansion fails boundedly before allocating huge blobs.
    std::string huge = "InlineCalls(p0,p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13); ";
    for(int i=0;i<14;++i) huge += "Program p"+std::to_string(i)+"("+std::to_string(2000+i)+") { " +
        (i==13 ? "print \"01234567890123456789\"; " :
         "Call(p"+std::to_string(i+1)+"); Call(p"+std::to_string(i+1)+"); ") + "}; ";
    rejects(huge);
    puts("InlineCalls: byte-equivalent explicit expansion, nested/repeated sites, retained conditional/loop semantics, cleanup identity and bounded rejection pass.");
}
