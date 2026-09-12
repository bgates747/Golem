#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    const std::string base =
        "Buffer b(1000,8); Field bitmap(b,0,u16,0,34); Field x(b,2,s16); Field y(b,4,s16); "
        "Matrix m(1100,3,3); ";
    auto plain = hosted::compile(base + "Program p(2000) { DrawBitmap(bitmap,x,y); };");
    const hosted::Bytes noAffine = {23,27,0,0,23,0,0x96,1,255,255,23,27,3,0,0,0,0,23,0,0x96,1,255,255};
    assert(std::search(plain.code.begin(), plain.code.end(), noAffine.begin(), noAffine.end()) != plain.code.end());
    auto affine = hosted::compile(base + "Program p(2000) { Identity(m); Scale(m,-0.5,0.5); "
                                          "Translate(m,50.5,0); DrawBitmap(bitmap,x,y,m); };");
    hosted::Bytes matrix = noAffine;matrix[8] = 1100 & 255;matrix[9] = 1100 >> 8;
    assert(std::search(affine.code.begin(), affine.code.end(), matrix.begin(), matrix.end()) != affine.code.end());
    assert(affine.map.find("matrix 1100 reset65535 external_range 0 34") != std::string::npos);
    (void)hosted::compile(base + "MatValues(m,1,0.5,0,0.25,1,0,0,0,1); DrawBitmap(bitmap,x,y,m);");
    auto reject = [](const std::string &s) {
        bool failed = false;
        try { (void)hosted::compile(s); } catch (const CompileError &) { failed = true; }
        assert(failed);
    };
    reject(base + "DrawBitmap(bitmap,x);");
    reject(base + "DrawBitmap(bitmap,bitmap,y);");
    reject(base + "DrawBitmap(x,x,y);");
    reject(base + "DrawBitmap(bitmap,x,y,m);");
    reject(base + "Identity(m); Scale(m,0,1); DrawBitmap(bitmap,x,y,m);");
    reject(base + "MatValues(m,1,0,0,0,1,0,0,0,0); DrawBitmap(bitmap,x,y,m);");
    reject(base + "MatValues(m,1,1,0,1,1,0,0,0,1); DrawBitmap(bitmap,x,y,m);");
    reject(base + "MatValues(m,1,0,0,0,1,0,0.5,0,1); DrawBitmap(bitmap,x,y,m);");
    reject(base + "Matrix n(1101,1,1); MatValues(n,1); DrawBitmap(bitmap,x,y,n);");
    reject(base + "Program other(2001) { Identity(m); }; "
                  "Program p(2000) { Identity(m); Call(other); DrawBitmap(bitmap,x,y,m); };");
    reject(base + "Field wide(b,6,u16); DrawBitmap(wide,x,y);");
    reject(base + "Field external(b,6,u8,101,101); Buffer owned(64101,2); DrawBitmap(external,x,y);");
    (void)hosted::compile(base + "Field external(b,6,u8,255,255); Buffer unrelated(64101,2); DrawBitmap(external,x,y);");
    reject(base + "MatValues(m,0.0000000000000000000000000000000000000001,0,0,0,1,0,0,0,1); DrawBitmap(bitmap,x,y,m);");
    puts("Bitmap command templates, affine proof, ownership, signed coordinates and rejection checks pass.");
}
