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
    auto r = hosted::compile(
        "Matrix a(1100,1,1); Matrix b(1101,1,1); Matrix c(1102,1,1); MatValues(a,7); "
        "MatValues(b,9); Program multiply(2000) { MatMul(c,a,b); }; ");
    auto u = uploads(r);
    assert(u[2000] == B({23, 0, 160, 78, 4, 34, 6, 1, 1, 76, 4, 77, 4}));
    assert(u[1102] == B({0, 0, 0, 0})); // No host-precomputed product is uploaded.
    auto load = uploads(hosted::compile("Buffer input(1000,4); Field x(input,0,s32,-10,10); Matrix "
                                        "m(1100,1,1); Program p(2000) { MatLoad(m,x); };"));
    assert(load[2000] == B({23, 0, 160, 76, 4, 34, 32, 1, 1, 64, 232, 3, 0, 0}));
    auto forward = uploads(hosted::compile(
        "Program first(2000) { Call(second); }; Program second(2001) { print \"ok\"; };"));
    assert(forward[2000] == B({23, 0, 160, 209, 7, 1}) && forward[2001] == B({'o', 'k'}));
    auto repeat = uploads(hosted::compile("Program p(2000) { Repeat(8) { print \"r\"; }; };"));
    assert(repeat[3] == B({8, 0})); // counter=2, limit=3, body=4
    assert(repeat[4].front() == 'r' && repeat[4].size() == 29);
    const B tail = {23, 0, 160, 4, 0, 6, 0xa4, 2, 0, 0, 0, 3, 0, 0, 0};
    assert(std::equal(tail.begin(), tail.end(), repeat[4].end() - tail.size()));
    auto empty = uploads(hosted::compile("Program p(2000) { Repeat(0) { print \"r\"; }; };"));
    assert(empty[2000].empty());
    auto floor = hosted::compile(
        "Buffer output(1500,4); Field x(output,0,s16); Field y(output,2,s16); Matrix m(1100,1,1); "
        "MatValues(m,-3.75); Program p(2000) { StoreI16Floor(x,m); PlotPoint(69,x,y); };");
    assert(uploads(floor)[2000].size() > 70 && floor.map.find("copy 2000") != std::string::npos);
    for (const char *bad :
         {"Buffer a(1000,4); Buffer b(1000,4);",
          "Buffer a(65535,4);",
          "Buffer a(1000,4); Field x(a,2,s32);",
          "Matrix a(1100,0,1);",
          "Matrix a(1100,9,9);",
          "Matrix a(1100,1,2); Matrix b(1101,1,2); Matrix c(1102,1,1); MatMul(c,a,b);",
          "Matrix a(1100,1,1); MatValues(a,1,2);",
          "Program p(2000) { Call(p); };",
          "Program p(2000) { Call(q); }; Program q(2001) { Call(p); };",
          "Program p(2000) { Repeat(513) { print \"x\"; }; };",
          "Buffer a(1000,4); Field x(a,0,s32); Set(x,2147483648);",
          "Buffer a(1000,4); Call(a);",
          "Buffer a(1000,4); Field x(a,0,u16); Program p(2000) { CallIf(p,x,lt,x); };",
          "Matrix a(1100,1,1); Matrix b(1101,1,1); MatMul(a,a,b);",
          "Matrix a(1100,3,3); Identity(a); Scale(a,0,1); InvertAffine(a);",
          "Buffer a(1000,2); Field x(a,0,s16); Matrix m(1100,1,1); MatValues(m,32768); "
          "StoreI16Floor(x,m);",
          "Buffer a(1000,2); Field x(a,0,s16); Matrix m(1100,1,1); StoreI16Floor(x,m);",
          "Buffer a(1000,4); Field x(a,0,u16); Matrix m(1100,1,1); MatLoad(m,x);",
          "Matrix a(1100,1,1); MatValues(a,1e999);",
          "Program p(2000) { Buffer x(1500,8); };",
          "Program p(2000) { Repeat(2) { Call(p); }; };",
          "Matrix m(1100,1,1); Field cell(m,0,f32); Set(cell,1);",
          "Buffer a(1000,4); Field x(a,0,s16,-1,1); Field y(a,2,s16); Copy(x,y,2);",
          "Buffer a(1000,2); Field x(a,0,s16); Field alias(a,0,u16); Set(x,1);",
          "Buffer a(1000,2); Field x(a,0,s16,-1,1); Matrix m(1100,1,1); "
          "MatValues(m,3); StoreI16Floor(x,m);",
          "Buffer a(1000,2); Field x(a,0,s16,-1,1); Matrix m(1100,1,1); "
          "Matrix n(1101,1,1); Field cell(m,0,f32,-1,1); MatValues(m,20); "
          "MatLoad(n,cell); StoreI16Floor(x,n);"})
        rejects(bad);
    auto rounded = hosted::plus(hosted::exact(16777216), hosted::exact(1));
    assert(rounded.lo <= 16777217 && rounded.hi >= 16777217 && rounded.lo != rounded.hi);
    puts("Hosted compiler: stock byte goldens, typed load, forward calls, finite repeat, "
         "signed-store/PLOT lowering and 26 negative cases pass.");
}
